/**
 *  \file soAllocDataCluster.c (implementation file)
 *
 *  \author
 */

#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#include "sofs_probe.h"
#include "sofs_buffercache.h"
#include "sofs_superblock.h"
#include "sofs_inode.h"
#include "sofs_datacluster.h"
#include "sofs_basicoper.h"
#include "sofs_basicconsist.h"
/* #define  CLEAN_CLUSTER */
#ifdef CLEAN_CLUSTER
#include "sofs_ifuncs_3.h"
#endif

/* Allusion to internal functions */

int soReplenish (SOSuperBlock *p_sb);
int soDeplete (SOSuperBlock *p_sb);

/**
 *  \brief Allocate a free data cluster and associate it to an inode.
 *
 *  The inode is supposed to be associated to a file (a regular file, a directory or a symbolic link), but the only
 *  consistency check at this stage should be to check if the inode is not free.
 *
 *  The cluster is retrieved from the retrieval cache of free data cluster references. If the cache is empty, it has to
 *  be replenished before the retrieval may take place. If the data cluster is in the dirty state, it has to be cleaned
 *  first. The header fields of the allocated cluster should be all filled in: <tt>prev</tt> and <tt>next</tt> should be
 *  set to \c NULL_CLUSTER and <tt>stat</tt> to the given inode number.
 *
 *  \param nInode number of the inode the data cluster should be associated to
 *  \param p_nClust pointer to the location where the logical number of the allocated data cluster is to be stored
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, the <em>inode number</em> is out of range or the <em>pointer to the logical data cluster
 *                      number</em> is \c NULL
 *  \return -\c ENOSPC, if there are no free data clusters
 *  \return -\c EIUININVAL, if the inode in use is inconsistent
 *  \return -\c EFDININVAL, if the free inode in the dirty state is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c EDCINVAL, if the data cluster header is inconsistent
 *  \return -\c EDCNOTIL, if the referenced data cluster is not in the list of direct references
 *  \return -\c EWGINODENB, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em>
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soAllocDataCluster (uint32_t nInode, uint32_t *p_nClust)
{
   soColorProbe (613, "07;33", "soAllocDataCluster (%"PRIu32", %p)\n", nInode, p_nClust);
  
  if(p_nClust == NULL)
  {
	  return -EINVAL;
  }
  
  SOSuperBlock *p_sb;		// Ponteiro para o SuperBlock
  SODataClust cluster;		// Criar um cluster
  int status;
  
  if((status = soLoadSuperBlock()) != 0) 	// Faz o load do SuperBlock para armazenamento interno
    return status; 
  
  p_sb = soGetSuperBlock();				// Adquire o ponteiro para o conteudo do SuperBlock 
  
  if((status = soQCheckDZ(p_sb)) != 0)	// Verifica a consistencia da DataZone 
    return status;
  
  if(p_sb->dZoneFree == 0)				// Verifica se existe data clusters livres 
    return -ENOSPC;
  
  if(p_sb->dZoneRetriev.cacheIdx == DZONE_CACHE_SIZE) {	
    if((status = soReplenish(p_sb)) !=0)					// Cache vazia entao replenish 	
      return status;
  }
  
  *p_nClust = p_sb->dZoneRetriev.cache[p_sb->dZoneRetriev.cacheIdx];
  p_sb->dZoneRetriev.cache[p_sb->dZoneRetriev.cacheIdx] = NULL_CLUSTER;
  p_sb->dZoneRetriev.cacheIdx += 1;
  p_sb->dZoneFree -= 1;
  
  cluster.prev = cluster.next = NULL_CLUSTER;
  cluster.stat = nInode;
  
  if((status=soStoreSuperBlock()) != 0) 
    return status;

  return 0;
}

/**
 *  \brief Replenish the retrieval cache of free data cluster references.
 *
 *  \param p_sb pointer to a buffer where the superblock data is stored
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soReplenish (SOSuperBlock *p_sb)
{

  /* insert your code here */

  return 0;
}
