/**
 *  \file soWriteFileCluster.c (implementation file)
 *
 *  \author
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
 *  \brief Write a specific data cluster.
 *
 *  Data is written into a specific data cluster which is supposed to belong to an inode
 *  associated to a file (a regular file, a directory or a symbolic link). Thus, the inode must be in use and belong
 *  to one of the legal file types.
 *
 *  If the cluster has not been allocated yet, it will be allocated now so that data can be stored there.
 *
 *  \param nInode number of the inode associated to the file
 *  \param clustInd index to the list of direct references belonging to the inode where data is to be written into
 *  \param buff pointer to the buffer where data must be written from
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>inode number</em> or the <em>index to the list of direct references</em> are out of
 *                      range or the <em>pointer to the buffer area</em> is \c NULL
 *  \return -\c EIUININVAL, if the inode in use is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c EDCINVAL, if the data cluster header is inconsistent
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soWriteFileCluster (uint32_t nInode, uint32_t clustInd, SODataClust *buff)
{
  soColorProbe (412, "07;31", "soWriteFileCluster (%"PRIu32", %"PRIu32", %p)\n", nInode, clustInd, buff);

  SODataClust *pCluster;
  SOSuperBlock *p_sosb;
  uint32_t nLogicClust;
  int stat;

  //Load do SuperBlock
  if((stat = soLoadSuperBlock()) != 0){
    return stat;
  }

  //Obter o ponteiro para o conteudo do SuperBlock
  p_sosb = soGetSuperBlock();

  //Validacao de conformidade
  if (((nInode >= p_sosb->iTotal)|| nInode <= 0) || (clustInd > MAX_FILE_CLUSTERS) || (buff == NULL)){
    return -EINVAL;
  }
  
  //Obter o numero logico do cluster
  if((stat = soHandleFileCluster (nInode, clustInd, GET, &nLogicClust)) != 0){
    return stat;
  }

  //Se o cluster ainda nao tiver sido alocado, utiliza-se a funcao handleFileCluster para o fazer
  if(nLogicClust == NULL_CLUSTER){
    if((stat = soHandleFileCluster (nInode, clustInd, ALLOC, &nLogicClust)) != 0){
      return stat;
    }
  }

  //Carrega o conteudo do cluster especifico
  if((stat = soLoadDirRefClust(p_sosb->dZoneStart+nLogicClust*BLOCKS_PER_CLUSTER)) != 0){
    return stat;
  }
  
  //Obter ponteiro para o cluster
  pCluster = soGetDirRefClust();
  
  //Copiar dados do buff para o cluster
  pCluster->info = buff->info;
  
  //Gravar cluster
  if((stat = soStoreDirRefClust()) != 0){
    return stat;
  }
  
  //Gravar SuperBlock
  if((stat = soStoreSuperBlock()) != 0){
    return stat;
  }
  
  return 0;

}
