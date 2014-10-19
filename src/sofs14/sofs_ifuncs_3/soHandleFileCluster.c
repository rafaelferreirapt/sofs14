/**
 *  \file soHandleFileCluster.c (implementation file)
 *
 *  \author Rafael Ferreira e Rodrigo Cunha
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include "sofs_probe.h"
#include "sofs_buffercache.h"
#include "sofs_superblock.h"
#include "sofs_inode.h"
#include "sofs_datacluster.h"
#include "sofs_basicoper.h"
#include "sofs_basicconsist.h"
#include "sofs_ifuncs_1.h"
#include "sofs_ifuncs_2.h"
#include "../sofs_ifuncs_3.h"

/** \brief operation get the logical number of the referenced data cluster for an inode in use */
#define GET         0
/** \brief operation allocate a new data cluster and associate it to the inode which describes the file */
#define ALLOC       1
/** \brief operation free the referenced data cluster */
#define FREE        2
/** \brief operation free the referenced data cluster and dissociate it from the inode which describes the file */
#define FREE_CLEAN  3
/** \brief operation dissociate the referenced data cluster from the inode which describes the file */
#define CLEAN       4

/* allusion to internal functions */

int soHandleDirect (SOSuperBlock *p_sb, uint32_t nInode, SOInode *p_inode, uint32_t nClust, uint32_t op,
    uint32_t *p_outVal);
int soHandleSIndirect (SOSuperBlock *p_sb, uint32_t nInode, SOInode *p_inode, uint32_t nClust, uint32_t op,
    uint32_t *p_outVal);
int soHandleDIndirect (SOSuperBlock *p_sb, uint32_t nInode, SOInode *p_inode, uint32_t nClust, uint32_t op,
    uint32_t *p_outVal);
int soAttachLogicalCluster (SOSuperBlock *p_sb, uint32_t nInode, uint32_t clustInd, uint32_t nLClust);
int soCleanLogicalCluster (SOSuperBlock *p_sb, uint32_t nInode, uint32_t nLClust);

/* funções auxiliares */
int sIndirect_FREE(SOInode *p_inode, SODataClust *p_clust, uint32_t clustInd);
int sIndirect_CLEAN(SOInode *p_inode, SODataClust *p_clust, uint32_t clustInd, uint32_t nInode,  SOSuperBlock *p_sb);
int dIndirect_FREE(SOInode *p_inode, SODataClust *p_clust, uint32_t clustInd, SOSuperBlock *p_sb, uint32_t l1, uint32_t l2);
int dIndirect_CLEAN(SOInode *p_inode, SODataClust *p_clust, uint32_t clustInd, uint32_t nInode,  SOSuperBlock *p_sb, uint32_t l1, uint32_t l2);

/**
 *  \brief Handle of a file data cluster.
 *
 *  The file (a regular file, a directory or a symlink) is described by the inode it is associated to.
 *
 *  Several operations are available and can be applied to the file data cluster whose logical number is given.
 *
 *  The list of valid operations is
 *
 *    \li GET:        get the logical number of the referenced data cluster for an inode in use
 *    \li ALLOC:      allocate a new data cluster and associate it to the inode which describes the file
 *    \li FREE:       free the referenced data cluster
 *    \li FREE_CLEAN: free the referenced data cluster and dissociate it from the inode which describes the file
 *    \li CLEAN:      dissociate the referenced data cluster from the inode which describes the file.
 *
 *  Depending on the operation, the field <em>cluCount</em> and the lists of direct references, single indirect
 *  references and double indirect references to data clusters of the inode associated to the file are updated.
 *
 *  Thus, the inode must be in use and belong to one of the legal file types for the operations GET, ALLOC, FREE and
 *  FREE_CLEAN and must be free in the dirty state for the operation CLEAN.
 *
 *  \param nInode number of the inode associated to the file
 *  \param clustInd index to the list of direct references belonging to the inode which is referred
 *  \param op operation to be performed (GET, ALLOC, FREE, FREE AND CLEAN, CLEAN)
 *  \param p_outVal pointer to a location where the logical number of the data cluster is to be stored
 *                  (GET / ALLOC); in the other cases (FREE / FREE AND CLEAN / CLEAN) it is not used
 *                  (in these cases, it should be set to \c NULL)
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>inode number</em> or the <em>index to the list of direct references</em> are out of
 *                      range or the requested operation is invalid or the <em>pointer to outVal</em> is \c NULL when it
 *                      should not be (GET / ALLOC)
 *  \return -\c EIUININVAL, if the inode in use is inconsistent
 *  \return -\c EFDININVAL, if the free inode in the dirty state is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c EDCINVAL, if the data cluster header is inconsistent
 *  \return -\c EDCARDYIL, if the referenced data cluster is already in the list of direct references (ALLOC)
 *  \return -\c EDCNOTIL, if the referenced data cluster is not in the list of direct references
 *              (FREE / FREE AND CLEAN / CLEAN)
 *  \return -\c EWGINODENB, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em> (ALLOC / FREE AND CLEAN / CLEAN)
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soHandleFileCluster (uint32_t nInode, uint32_t clustInd, uint32_t op, uint32_t *p_outVal)
{
  soColorProbe (413, "07;31", "soHandleFileCluster (%"PRIu32", %"PRIu32", %"PRIu32", %p)\n",
      nInode, clustInd, op, p_outVal);

  /* Validação de conformidade 
     - a operação solicitadea tem que ser válida 
   */
  if(op != GET && op != ALLOC && op != FREE && op != FREE_CLEAN && op != CLEAN){
    return -EINVAL;
  }

  /* se GET e ALLOC então *p_outVal != NULL */
  if(op == GET || op == ALLOC){
    if(p_outVal == NULL){
      return -EINVAL;
    }
  }

  int stat;
  SOSuperBlock* p_sb;
  SOInode *p_inode = NULL;

  if((stat = soLoadSuperBlock()) != 0){
    return stat;
  }

  p_sb = soGetSuperBlock();

  if(nInode >= p_sb->iTotal || nInode < 0){
    return -EINVAL;
  }

  /* CONFERIR SE É MAIOR OU MAIOR E IGUAL o clustInd */
  if(clustInd < 0 || clustInd > MAX_FILE_CLUSTERS){
    return -EINVAL;
  }

  if(op == CLEAN){
    if((stat = soReadInode(p_inode, nInode, FDIN)) != 0){
      return stat;
    }
  }else{
    if((stat = soReadInode(p_inode, nInode, IUIN)) != 0){
      return stat;
    }
  }

  if(clustInd <= N_DIRECT){
    if((stat = soHandleDirect(p_sb, nInode, p_inode, clustInd, op, p_outVal))){
      return stat;
    }
    /* RPC => number of data cluster references per data cluster */
    /* referência indireta => i1 */
  }else if(clustInd <= (N_DIRECT + RPC)){
    if((stat = soHandleSIndirect(p_sb, nInode, p_inode, clustInd, op, p_outVal))){
      return stat;
    }
    /* referência duplamente indirecta => i1 + i2 */
  }else if(clustInd <= MAX_FILE_CLUSTERS){
    if((stat = soHandleDIndirect(p_sb, nInode, p_inode, clustInd, op, p_outVal))){
      return stat;
    }
  }

  if(GET){
    return 0;
  }

  if(op != GET){
    if((stat = soWriteInode(p_inode, nInode, IUIN)) != 0){
      return stat;
    }
  }
  
  if(op == ALLOC){
    if ((stat = soAttachLogicalCluster(p_sb, nInode, clustInd, *p_outVal)) != 0){
      return stat;
    }
  }

  if((stat = soStoreSuperBlock())){
    return stat;
  }

  return 0;
}

/**
 *  \brief Handle of a file data cluster which belongs to the direct references list.
 *
 *  \param p_sb pointer to a buffer where the superblock data is stored
 *  \param nInode number of the inode associated to the file
 *  \param p_inode pointer to a buffer which stores the inode contents
 *  \param clustInd index to the list of direct references belonging to the inode which is referred
 *  \param op operation to be performed (GET, ALLOC, FREE, FREE AND CLEAN, CLEAN)
 *  \param p_outVal pointer to a location where the logical number of the data cluster is to be stored
 *                  (GET / ALLOC); in the other cases (FREE / FREE AND CLEAN / CLEAN) it is not used
 *                  (in these cases, it should be set to \c NULL)
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the requested operation is invalid
 *  \return -\c EDCARDYIL, if the referenced data cluster is already in the list of direct references (ALLOC)
 *  \return -\c EDCNOTIL, if the referenced data cluster is not in the list of direct references
 *              (FREE / FREE AND CLEAN / CLEAN)
 *  \return -\c EWGINODENB, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em> (FREE AND CLEAN / CLEAN)
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soHandleDirect (SOSuperBlock *p_sb, uint32_t nInode, SOInode *p_inode, uint32_t clustInd, uint32_t op,
    uint32_t *p_outVal)
{
  uint32_t cluster, stat, p_stat;

  switch(op){
    case GET:
      *p_outVal = p_inode->d[clustInd];
      break;
    case ALLOC:
      /* if the referenced data cluster is already in the list of direct references (ALLOC) */
      if(p_inode->d[clustInd] != NULL_CLUSTER){
        return -EDCARDYIL;
      }

      if((stat = soAllocDataCluster(nInode, &cluster))){
        return stat; 
      }

      p_inode->d[clustInd] = cluster;
      p_inode->cluCount++;
      p_outVal = &cluster;
      break;
    case FREE:
      if(p_inode->d[clustInd] == NULL_CLUSTER){
        return -EDCNOTIL;
      }
      if((stat = soFreeDataCluster(p_inode->d[clustInd]))){
        return stat;
      }
      break;
    case FREE_CLEAN:
      if(p_inode->d[clustInd] == NULL_CLUSTER){
        return -EDCNOTIL;
      }
      if((stat = soFreeDataCluster(p_inode->d[clustInd]))){
        return stat;
      }
      if((stat = soCleanLogicalCluster(p_sb, nInode, p_inode->d[clustInd]))){
        return stat;
      }
      p_inode->cluCount--;
      break;
    case CLEAN:
      if(p_inode->d[clustInd] == NULL_CLUSTER){
        return -EDCNOTIL;
      }

      if((stat = soQCheckStatDC(p_sb, p_inode->d[clustInd], &p_stat))){
        return stat;
      }

      if(p_stat != FREE_CLT){
        return -EFDININVAL;
      }
        
      if((stat = soCleanLogicalCluster(p_sb, nInode, p_inode->d[clustInd]))){
        return stat;
      }
      p_inode->cluCount--;
      break;
    }

  return 0;
}

/**
 *  \brief Handle of a file data cluster which belongs to the single indirect references list.
 *
 *  \param p_sb pointer to a buffer where the superblock data is stored
 *  \param nInode number of the inode associated to the file
 *  \param p_inode pointer to a buffer which stores the inode contents
 *  \param clustInd index to the list of direct references belonging to the inode which is referred
 *  \param op operation to be performed (GET, ALLOC, FREE, FREE AND CLEAN, CLEAN)
 *  \param p_outVal pointer to a location where the logical number of the data cluster is to be stored
 *                  (GET / ALLOC); in the other cases (FREE / FREE AND CLEAN / CLEAN) it is not used
 *                  (in these cases, it should be set to \c NULL)
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the requested operation is invalid
 *  \return -\c EDCARDYIL, if the referenced data cluster is already in the list of direct references (ALLOC)
 *  \return -\c EDCNOTIL, if the referenced data cluster is not in the list of direct references
 *              (FREE / FREE AND CLEAN / CLEAN)
 *  \return -\c EWGINODENB, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em> (FREE AND CLEAN / CLEAN)
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soHandleSIndirect (SOSuperBlock *p_sb, uint32_t nInode, SOInode *p_inode, uint32_t clustInd, uint32_t op,
    uint32_t *p_outVal)
{
  SODataClust *p_clust;
  uint32_t clustRef, stat, nClust;

  if(p_inode->i1 != NULL_CLUSTER){
    clustRef = p_sb->dZoneStart + p_inode->i1 * BLOCKS_PER_CLUSTER;
    if((stat = soLoadDirRefClust(clustRef))){
      return stat;
    }
    p_clust = soGetDirRefClust();
  }

  switch(op){
    case GET:
      if(p_inode->i1 == NULL_CLUSTER){
        *p_outVal = NULL_CLUSTER;
      }else{
        *p_outVal = p_clust->info.ref[clustInd];
      }
      break;
    case ALLOC:

      /* se o p_inode->i1 for NULL_CLUSTER temos de alocar um cluster para extender
         o D e colocar todas as refs a NULL_CLUSTER */
      if(p_inode->i1==NULL_CLUSTER){
        if((stat = soAllocDataCluster(nInode,&nClust))){
          return stat;
        }

        p_inode->i1 = nClust;
        p_inode->cluCount++;

        clustRef = p_sb->dZoneStart + nClust * BLOCKS_PER_CLUSTER;
        if((stat = soLoadDirRefClust(clustRef))){
          return stat;
        }
        
        p_clust = soGetDirRefClust();

        int i;
        for(i=0; i<RPC; i++){
          p_clust->info.ref[i] = NULL_CLUSTER;
        }
      }   

      /* se queremos alocar, tem de ser nulo obrigatoriamente */
      if(p_clust->info.ref[clustInd] != NULL_CLUSTER){
        return -EDCARDYIL;
      }
      /* vamos alocar um cluster para o nInode */
      if((stat = soAllocDataCluster(nInode, &nClust))){
        return stat;
      }

      p_clust->info.ref[clustInd] = nClust;
      /*
      if((stat = soAttachLogicalCluster(p_sb, nInode, clustInd, nClust))){
        return stat;
      }
      */
      p_inode->cluCount++;

      if((stat = soStoreDirRefClust())){
        return stat;
      }

      *p_outVal = nClust;


      break;
    case FREE:
      if((stat = sIndirect_FREE(p_inode, p_clust, clustInd))){
        return stat;
      }
      break;
    case FREE_CLEAN:
      if((stat = sIndirect_FREE(p_inode, p_clust, clustInd))){
        return stat;
      }
      if((stat = sIndirect_CLEAN(p_inode, p_clust, clustInd, nInode, p_sb))){
        return stat;
      }
      break;
    case CLEAN:
      if((stat = sIndirect_CLEAN(p_inode, p_clust, clustInd, nInode, p_sb))){
        return stat;
      }
      break;
  }

  return 0;
}

int sIndirect_FREE(SOInode *p_inode, SODataClust *p_clust, uint32_t clustInd){
  int stat;
  if(p_inode->i1 == NULL_CLUSTER || p_clust->info.ref[clustInd] == NULL_CLUSTER){
    return -EDCNOTIL;
  }
  if((stat = soFreeDataCluster(p_clust->info.ref[clustInd]))){
    return stat;
  }
  return 0;
}

int sIndirect_CLEAN(SOInode *p_inode, SODataClust *p_clust, uint32_t clustInd, uint32_t nInode, SOSuperBlock *p_sb){
  int stat;
  if(p_inode->i1 == NULL_CLUSTER || p_clust->info.ref[clustInd] == NULL_CLUSTER){
    return -EDCNOTIL;
  }
  if((stat = soCleanLogicalCluster(p_sb, nInode, p_clust->info.ref[clustInd]))){
    return stat;
  }

  p_clust->info.ref[clustInd] = NULL_CLUSTER;
  p_inode->cluCount--;

  int i = 0;
  for(; i<RPC && p_clust->info.ref[i] == NULL_CLUSTER; i++){
    continue;
  }

  /* se forem tods null_cluster não faz sentido ter o i1 */
  if(i == RPC){
    if((stat = soFreeDataCluster(p_inode->i1))){
      return stat;
    }
    if((stat = soCleanLogicalCluster(p_sb, nInode, p_inode->i1))){
      return stat;
    }
    p_inode->i1 = NULL_CLUSTER;
    p_inode->cluCount--;
  }

  if((stat = soStoreDirRefClust())){
    return stat;
  }

  return 0;
}



/**
 *  \brief Handle of a file data cluster which belongs to the double indirect references list.
 *
 *  \param p_sb pointer to a buffer where the superblock data is stored
 *  \param nInode number of the inode associated to the file
 *  \param p_inode pointer to a buffer which stores the inode contents
 *  \param clustInd index to the list of direct references belonging to the inode which is referred
 *  \param op operation to be performed (GET, ALLOC, FREE, FREE AND CLEAN, CLEAN)
 *  \param p_outVal pointer to a location where the logical number of the data cluster is to be stored
 *                  (GET / ALLOC); in the other cases (FREE / FREE AND CLEAN / CLEAN) it is not used
 *                  (in these cases, it should be set to \c NULL)
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the requested operation is invalid
 *  \return -\c EDCARDYIL, if the referenced data cluster is already in the list of direct references (ALLOC)
 *  \return -\c EDCNOTIL, if the referenced data cluster is not in the list of direct references
 *              (FREE / FREE AND CLEAN / CLEAN)
 *  \return -\c EWGINODENB, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em> (FREE AND CLEAN / CLEAN)
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soHandleDIndirect (SOSuperBlock *p_sb, uint32_t nInode, SOInode *p_inode, uint32_t clustInd, uint32_t op,
    uint32_t *p_outVal)
{

  uint32_t l2, l1;
  int stat;
  SODataClust *p_clust = NULL;
  uint32_t *nClust = NULL;

  if(op != GET && op != ALLOC && op != FREE && op != FREE_CLEAN && op != CLEAN){
    return -EINVAL;
  }

  l2 = (clustInd - N_DIRECT - RPC) / RPC;
  l1 = (clustInd - N_DIRECT - RPC) % RPC;

  switch(op){
    case GET:
    {
      if(p_inode->i2 == NULL_CLUSTER){
        *p_outVal = NULL_CLUSTER;
      }else{
        if((stat = soLoadSngIndRefClust(p_sb->dZoneStart + p_inode->i2 * BLOCKS_PER_CLUSTER)) != 0){
          return stat;
        }

        p_clust = soGetSngIndRefClust();

        if(p_clust->info.ref[l2] == NULL_CLUSTER){
          *p_outVal = NULL_CLUSTER;
        }else{
          if((stat = soLoadDirRefClust(p_sb->dZoneStart + p_clust->info.ref[l2] * BLOCKS_PER_CLUSTER)) != 0){
            return stat;
          }
          
          p_clust = soGetDirRefClust();

          *p_outVal = p_clust->info.ref[l1];
        }
      }
    break;
    }
    case ALLOC:
    {
      if(p_inode->i2 == NULL_CLUSTER){
        if((stat = soAllocDataCluster(nInode, nClust)) != 0){
          return stat;
        }
        
        p_inode->i2 = *nClust;

        if((stat = soReadCacheCluster(p_sb->dZoneStart + p_inode->i2 * BLOCKS_PER_CLUSTER, p_clust)) != 0){
          return stat;
        }
                    
        uint32_t i;
        for(i = 0; i < RPC; i++){
          p_clust->info.ref[i] = NULL_CLUSTER;
        }

        if((stat = soWriteCacheCluster(p_sb->dZoneStart + p_inode->i2 * BLOCKS_PER_CLUSTER, p_clust)) != 0){
          return stat;
        }
                    
        p_inode->cluCount++;
      }

      if((stat = soLoadSngIndRefClust(p_sb->dZoneStart + p_inode->i2 * BLOCKS_PER_CLUSTER)) != 0){
        return stat;
      }
                
      p_clust = soGetSngIndRefClust();

      if(p_clust->info.ref[l2] == NULL_CLUSTER){
        if((stat = soAllocDataCluster(nInode, nClust)) != 0){
          return stat;
        }
        
        p_clust->info.ref[l2] = *nClust;

        if((stat = soReadCacheCluster(p_sb->dZoneStart + p_clust->info.ref[l2] * BLOCKS_PER_CLUSTER, p_clust)) != 0){
          return stat;
        }
                    
        uint32_t i;
        for(i = 0; i < RPC; i++){
          p_clust->info.ref[i] = NULL_CLUSTER;
        }

        if((stat = soWriteCacheCluster(p_sb->dZoneStart + p_clust->info.ref[l2] * BLOCKS_PER_CLUSTER, p_clust)) != 0){
          return stat;
        }
        
        p_inode->cluCount++;
      }
      
      if((stat = soAllocDataCluster(nInode, nClust)) != 0){
        return stat;
      }
      
      if(p_clust->info.ref[l1] != NULL_CLUSTER){
        return -EDCARDYIL;
      }

      p_clust->info.ref[l1] = *p_outVal = *nClust;
      /*
      if((stat = soAttachLogicalCluster(p_sb, nInode, clustInd, p_clust->info.ref[l1])) != 0){
        return stat;
      }
      */
      p_inode->cluCount++;

      if ((stat = soStoreSngIndRefClust()) != 0){
        return stat;
      }
    }
    case FREE:
    {
      p_outVal = NULL;

      if((stat = dIndirect_FREE(p_inode, p_clust, clustInd, p_sb, l1, l2))){
        return stat;
      }

      p_inode->cluCount--;
    break;
    }
    case FREE_CLEAN:
    {
      p_outVal = NULL;

      if((stat = dIndirect_FREE(p_inode, p_clust, clustInd, p_sb, l1, l2))){
        return stat;
      }
      if((stat = dIndirect_CLEAN(p_inode, p_clust, clustInd, nInode, p_sb, l1, l2))){
        return stat;
      }

      p_inode->cluCount--;
    break;
    }
    case CLEAN:
    {
      p_outVal = NULL;     

      if((stat = dIndirect_CLEAN(p_inode, p_clust, clustInd, nInode, p_sb, l1, l2))){
        return stat;
      }

      p_inode->cluCount--;

    break;
    }
        default:
        {
            p_outVal = NULL;
            return -EINVAL;

        }
    }
    return 0;
}

int dIndirect_FREE(SOInode *p_inode, SODataClust *p_clust, uint32_t clustInd, SOSuperBlock *p_sb, uint32_t l1, uint32_t l2){
  
  int stat;

  if(p_inode->i2 == NULL_CLUSTER){
    return -EDCNOTIL;
  }

  if((stat = soLoadSngIndRefClust(p_sb->dZoneStart + p_inode->i2 * BLOCKS_PER_CLUSTER))){
    return stat;
  }
                
  p_clust = soGetSngIndRefClust();

  if(p_clust->info.ref[l2] == NULL_CLUSTER){
    return -EDCNOTIL;
  }

  if((stat = soLoadDirRefClust(p_sb->dZoneStart + p_clust->info.ref[l2] * BLOCKS_PER_CLUSTER))){
    return stat;
  }
                
  p_clust = soGetDirRefClust();

  if(p_clust->info.ref[l1] == NULL_CLUSTER) {
    return -EDCNOTIL;
  }
  
  if((stat = soFreeDataCluster(p_clust->info.ref[l1]))){
      return stat;
  }
  
  p_inode->cluCount--;

  if((stat = soStoreSngIndRefClust()) != 0){
    return stat;
  }

  return 0;

}


int dIndirect_CLEAN(SOInode *p_inode, SODataClust *p_clust, uint32_t clustInd, uint32_t nInode,  SOSuperBlock *p_sb, uint32_t l1, uint32_t l2){
  
  int stat;

  if(p_inode->i2 == NULL_CLUSTER){ 
    return -EDCNOTIL;
  }

  if((stat = soLoadSngIndRefClust(p_sb->dZoneStart + p_inode->i2 * BLOCKS_PER_CLUSTER))){
    return stat;
  }

  p_clust = soGetSngIndRefClust();

  if(p_clust->info.ref[l2] == NULL_CLUSTER){
    return -EDCNOTIL;
  }

  if((stat = soLoadDirRefClust(p_sb->dZoneStart + p_inode->i2 * BLOCKS_PER_CLUSTER))){
    return stat;
  }

  p_clust = soGetDirRefClust();

  if(p_clust->info.ref[l1] == NULL_CLUSTER) {
    return -EDCNOTIL;
  }
  
  if((stat = soCleanDataCluster(nInode, p_clust->info.ref[l1]))){
    return stat;
  }

  p_clust->info.ref[l1] = NULL_CLUSTER;

  return 0;
}

/**
 *  \brief Attach a file data cluster whose index to the list of direct references and logical number are known.
 *
 *  \param p_sb pointer to a buffer where the superblock data is stored
 *  \param nInode number of the inode associated to the file
 *  \param clustInd index to the list of direct references belonging to the inode which is referred
 *  \param nLClust logical number of the data cluster
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EWGINODENB, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em>
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soAttachLogicalCluster (SOSuperBlock *p_sb, uint32_t nInode, uint32_t clustInd, uint32_t nLClust)
{

  int stat;
  uint32_t NFClt, prev_ind, next_ind;
  SODataClust p_clust;

  /*Numero fisico do cluster alocado*/
  NFClt = p_sb->dZoneStart + nLClust * BLOCKS_PER_CLUSTER;

  if((stat = soHandleFileCluster(nInode, clustInd - 1, GET, &prev_ind))){
    return stat;
  }

  if((stat = soHandleFileCluster(nInode, clustInd + 1, GET, &next_ind))){
    return stat;
  }

  if((stat = soReadCacheCluster(NFClt, &p_clust)) != 0){
        return stat;
  }

  if(prev_ind != NULL_CLUSTER){
    p_clust.prev = prev_ind; 
  }

  if(next_ind != NULL_CLUSTER){
    p_clust.next = next_ind;
  }

  if((stat = soWriteCacheCluster(NFClt, &p_clust))){
    return stat;
  }

  return 0;
}

/**
 *  \brief Clean a file data cluster whose logical number is known.
 *
 *  \param p_sb pointer to a buffer where the superblock data is stored
 *  \param nInode number of the inode associated to the file
 *  \param nLClust logical number of the data cluster
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EWGINODENB, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em>
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soCleanLogicalCluster (SOSuperBlock *p_sb, uint32_t nInode, uint32_t nLClust)
{

  uint32_t NFClt;
  int stat;
  SODataClust p_clust;

  /*Numero fisico do cluster alocado*/
  NFClt = p_sb->dZoneStart + nLClust * BLOCKS_PER_CLUSTER;

  /*update campo stat*/
  if((stat = soReadCacheCluster(NFClt, &p_clust))){
    return stat;
  }
  if(p_clust.stat != nInode){
    return -EWGINODENB;
  } 

  p_clust.stat = NULL_INODE;

  memset(p_clust.info.data, 0, BSLPC); /*limpar datacluster*/

  if((stat = soWriteCacheCluster(NFClt, &p_clust))){
    return stat;
  }

  return 0;
}

