/**
 *  \file soHandleFileClusters.c (implementation file)
 *
 *  \author
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>

#include "sofs_probe.h"
#include "sofs_buffercache.h"
#include "sofs_superblock.h"
#include "sofs_inode.h"
#include "sofs_datacluster.h"
#include "sofs_basicoper.h"
#include "sofs_basicconsist.h"
#include "sofs_ifuncs_1.h"
#include "sofs_ifuncs_2.h"

/** \brief operation get the physical number of the referenced data cluster */
#define GET         0
/** \brief operation allocate a new data cluster and associate it to the inode which describes the file */
#define ALLOC       1
/** \brief operation free the referenced data cluster */
#define FREE        2
/** \brief operation free the referenced data cluster and dissociate it from the inode which describes the file */
#define FREE_CLEAN  3
/** \brief operation dissociate the referenced data cluster from the inode which describes the file */
#define CLEAN       4

/* allusion to internal function */

int soHandleFileCluster (uint32_t nInode, uint32_t clustInd, uint32_t op, uint32_t *p_outVal);

/**
 *  \brief Handle all data clusters from the list of references starting at a given point.
 *
 *  The file (a regular file, a directory or a symlink) is described by the inode it is associated to.
 *
 *  Several operations are available and can be applied to the file data clusters starting from the index to the list of
 *  direct references which is given.
 *
 *  The list of valid operations is
 *
 *    \li FREE:       free all data clusters starting from the referenced data cluster
 *    \li FREE_CLEAN: free all data clusters starting from the referenced data cluster and dissociate them from the
 *                    inode which describes the file
 *    \li CLEAN:      dissociate all data clusters starting from the referenced data cluster from the inode which
 *                    describes the file.
 *
 *  Depending on the operation, the field <em>clucount</em> and the lists of direct references, single indirect
 *  references and double indirect references to data clusters of the inode associated to the file are updated.
 *
 *  Thus, the inode must be in use and belong to one of the legal file types for the operations FREE and FREE_CLEAN and
 *  must be free in the dirty state for the operation CLEAN.
 *
 *  \param nInode number of the inode associated to the file
 *  \param clustIndIn index to the list of direct references belonging to the inode which is referred (it contains the
 *                    index of the first data cluster to be processed)
 *  \param op operation to be performed (FREE, FREE AND CLEAN, CLEAN)
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>inode number</em> or the <em>index to the list of direct references</em> are out of
 *                      range or the requested operation is invalid
 *  \return -\c EIUININVAL, if the inode in use is inconsistent
 *  \return -\c EFDININVAL, if the free inode in the dirty state is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c EDCINVAL, if the data cluster header is inconsistent
 *  \return -\c EWGINODENB, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em> (FREE AND CLEAN / CLEAN)
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soHandleFileClusters (uint32_t nInode, uint32_t clustIndIn, uint32_t op)
{
  soColorProbe (414, "07;31", "soHandleFileClusters (%"PRIu32", %"PRIu32", %"PRIu32")\n", nInode, clustIndIn, op);

  SOSuperBlock* p_sb;
  SOInode inode;
  SODataClust *clust1, *clust2;
  uint32_t stat, ind, ref_offset, ref_Soffset, ref_Doffset;

  if((stat = soLoadSuperBlock())){
  	return stat;
  }

  p_sb = soGetSuperBlock();

  if(nInode < 0 || nInode >= p_sb->iTotal){
  	return -EINVAL;
  }

  if(clustIndIn >= MAX_FILE_CLUSTERS){
  	return -EINVAL;
  }

  if(op < 2 || op > 4){
  	return -EINVAL;
  }

  if(op == CLEAN){
    if ((stat=soReadInode(&inode,nInode,FDIN))!=0){
      return stat;
    }
    status = FDIN;
  }else{
    if ((stat=soReadInode(&inode,nInode,IUIN))!=0){
      return stat;
    }
    status = IUIN;
  }

  /*Referencas Duplamente Indirectas*/

  if(inode.i2 != NULL_CLUSTER){

    if ((stat = soLoadSngIndRefClust((inode.i2 * BLOCKS_PER_CLUSTER) + p_sb->dZoneStart)) != 0){
      return stat; 
    }

    clust2 = soGetSngIndRefClust(); //pointer to the contents of a specific cluster of the table of single indirect references

    ind = N_DIRECT + RPC; //tamanho da tabela das referencias simplesmentre indirectas

    for(;inode.i2 != NULL_CLUSTER && ind < MAX_FILE_CLUSTERS;){
      ref_Soffset = (ind - (RPC + N_DIRECT)) / RPC;
      ref_Doffset = (ind - N_DIRECT - RPC) % RPC;

      if(clust2->info.ref[ref_Soffset] != NULL_CLUSTER){
        if((stat = soLoadDirRefClust((clust2->info.ref[ref_Soffset]* BLOCKS_PER_CLUSTER) + p_sb->dZoneStart)) != 0){
          return stat;
        }

        clust1 = soGetDirRefClust();

        for(; ref_Doffset < RPC; ref_Doffset++, ind++){
          if(clust1->info.ref[ref_Doffset] != NULL_CLUSTER && clustIndIn <= ind){
            if((stat = soHandleFileCluster(nInode, ind, op, NULL)) != 0){
              return stat;
            }
          }
        }
      }else{
        ind += RPC;
      }
    }
  }

  /*Referencias Simplesmente Indirectas*/

  if(inode.i1 != NULL_CLUSTER){
    if((stat = soLoadDirRefClust((inode.i1 * BLOCKS_PER_CLUSTER) + p_sb->dZoneStart)) != 0){
      return stat;
    }
    clust1  = soGetDirRefClust();

    ind = N_DIRECT;

    for(; inode.i1 != NULL_CLUSTER && ind < N_DIRECT + RPC;ind++){
      ref_offset = ind - N_DIRECT;

      if(clust1->info.ref[ref_offset] != NULL_CLUSTER && clustIndIn <= ind){
        if((stat = soHandleFileCluster(nInode, ind, op, NULL)) != 0 ){
          return stat;
        }
      }
    }
  }

  /*Referencias Directas*/

  ind = 0;

  for(; ind < N_DIRECT; ind++){
    if(inode.d[ind] != NULL_CLUSTER && clustIndIn <= ind){
      if((stat = soHandleFileCluster(nInode, ind, op, NULL)) != 0){
        return stat;
      }
    }
  }

  return 0;
}
