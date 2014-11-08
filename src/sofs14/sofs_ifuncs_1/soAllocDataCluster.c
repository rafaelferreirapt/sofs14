/**
 *  \file soAllocDataCluster.c (implementation file)
 *
 *  \author: soReplenish Xavier
 * 			 soAllocDataCluster José Silva
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
/* #ifdef CLEAN_CLUSTER */
#include "sofs_ifuncs_3.h"
/* #endif */

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

	
	SOSuperBlock *p_sb;		// Ponteiro para o SuperBlock
	SODataClust cluster;		// Criar um cluster
	int status;
	SOInode *inode;
	uint32_t nBlk, offset;


  if(p_nClust == NULL)
  {
    return -EINVAL;
  }

	if((status = soLoadSuperBlock()) != 0) 	// Faz o load do SuperBlock para armazenamento interno
		return status; 

	p_sb = soGetSuperBlock();				// Adquire o ponteiro para o conteudo do SuperBlock 

	if((status = soQCheckDZ(p_sb)) != 0)	// Verifica a consistencia da DataZone 
		return status;

	if(p_sb->dZoneFree == 0)				// Verifica se existe data clusters livre
		return -ENOSPC;

	if((status=soConvertRefInT(nInode,&nBlk,&offset))!=0)
	{
		return status;
	}

	if((status=soLoadBlockInT(nBlk))!=0)
	{
		return status;
	}

	inode=soGetBlockInT();

	if(soQCheckFInode(&inode[offset])==0)
	{
		return -EIUININVAL;
	}

	if(p_sb->dZoneRetriev.cacheIdx == DZONE_CACHE_SIZE) 
	{	
		if((status = soReplenish(p_sb)) !=0)					// Cache vazia entao replenish 	
			return status;
	}

	*p_nClust = p_sb->dZoneRetriev.cache[p_sb->dZoneRetriev.cacheIdx];
	p_sb->dZoneRetriev.cache[p_sb->dZoneRetriev.cacheIdx] = NULL_CLUSTER;
	p_sb->dZoneRetriev.cacheIdx += 1;
	p_sb->dZoneFree -= 1;

	if((status = soReadCacheCluster(p_sb->dZoneStart+(*p_nClust*BLOCKS_PER_CLUSTER),&cluster)) != 0)
	{	// Escrita no cluster
		return status;
	}
	cluster.prev = cluster.next = NULL_CLUSTER;
	
	if(cluster.stat != NULL_INODE){ /* it is, clean it */
		if ((status = soCleanDataCluster (cluster.stat, *p_nClust)) != 0){
   			return status;
		}
	}


	cluster.stat = nInode;
	
	if((status = soWriteCacheCluster(p_sb->dZoneStart+(*p_nClust*BLOCKS_PER_CLUSTER),&cluster)) != 0)
	{	// Escrita no cluster
		return status;
	}

	if((status=soStoreSuperBlock()) != 0)
	{ 
		return status;
	}
	
	
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

	uint32_t nctt;
	if(p_sb == NULL)
	{
		return -EINVAL;
	}


	if ( p_sb->dZoneFree < DZONE_CACHE_SIZE )
	{
		nctt = p_sb->dZoneFree;
	}

	else
	{
		nctt = DZONE_CACHE_SIZE;
	}

	uint32_t nLCluster = p_sb->dHead;
	int result = 0;
	SODataClust dc;		/*invocação de um cluster*/
	int n;

	for (n = DZONE_CACHE_SIZE - nctt; n < DZONE_CACHE_SIZE; n++)
	{
		if ( nLCluster == NULL_CLUSTER)
			break;
		p_sb->dZoneRetriev.cache[n] = nLCluster;
		if((result=soReadCacheCluster((p_sb->dZoneRetriev.cache[n]*BLOCKS_PER_CLUSTER)+p_sb->dZoneStart, &dc))!=0)
		{
			return result;
		}
		nLCluster =  dc.next;
		dc.prev = dc.next = NULL_CLUSTER;
		if((result=soWriteCacheCluster((p_sb->dZoneRetriev.cache[n]*BLOCKS_PER_CLUSTER)+p_sb->dZoneStart,&dc))!=0)
		{
			return result;
		}
	}

	if (n != DZONE_CACHE_SIZE)
	{
		p_sb->dHead = p_sb->dTail = NULL_CLUSTER;

		if((result = soDeplete(p_sb))!=0)
		{
			return result;
		}

		nLCluster = p_sb->dHead;

		for ( ; n < DZONE_CACHE_SIZE; n++)
		{
			p_sb->dZoneRetriev.cache[n] = nLCluster;	/*atribuição do cluster anterior para a cache de retirada*/
			if((result=soReadCacheCluster((p_sb->dZoneRetriev.cache[n]*BLOCKS_PER_CLUSTER)+p_sb->dZoneStart,&dc))!=0)
			{
				return result;
			}
			nLCluster = dc.next;	/*nLCluster fica com o cluster actual*/
			dc.prev = dc.next = NULL_CLUSTER;
			if((result=soWriteCacheCluster((p_sb->dZoneRetriev.cache[n]*BLOCKS_PER_CLUSTER)+p_sb->dZoneStart,&dc))!=0)
			{
				return result;
			}
		}
	}
	if (nLCluster == NULL_CLUSTER)
	{
		p_sb->dTail = NULL_CLUSTER;
		p_sb->dHead = NULL_CLUSTER;
	}
	else
	{
		p_sb->dHead = nLCluster;
	}

	if (nLCluster != NULL_CLUSTER)
	{
		if((result=soReadCacheCluster((p_sb->dHead*BLOCKS_PER_CLUSTER)+p_sb->dZoneStart,&dc))!=0)
		{
			return result;
		}
		dc.prev = NULL_CLUSTER;
		if((result=soWriteCacheCluster((p_sb->dHead*BLOCKS_PER_CLUSTER)+p_sb->dZoneStart,&dc))!=0)
		{
			return result;
		}
	}
	p_sb->dZoneRetriev.cacheIdx = DZONE_CACHE_SIZE - nctt;
	return 0;
}
