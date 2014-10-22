/**
 *  \file soCleanDataCluster.c (implementation file)
 *
 *  \author
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>

#include "sofs_probe.h"
#include "sofs_buffercache.h"
#include "../sofs_superblock.h"
#include "../sofs_inode.h"
#include "../sofs_datacluster.h"
#include "../sofs_basicoper.h"
#include "../sofs_basicconsist.h"
#include "../sofs_ifuncs_1.h"
#include "../sofs_ifuncs_2.h"

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
 *  \brief Clean a data cluster from the inode describing a file which was previously deleted.
 *
 *  The inode is supposed to be free in the dirty state.
 *
 *  The list of references is parsed until the logical number of the data cluster is found or until the list is
 *  exhausted. If found, the data cluster (and all data clusters in its dependency, if it belongs to the auxiliary
 *  data structure that entails the list of single indirect or double indirect references) is cleaned.
 *
 *  \param nInode number of the inode associated to the data cluster
 *  \param nLClust logical number of the data cluster
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>inode number</em> or the <em>logical cluster number</em> are out of range
 *  \return -\c EFDININVAL, if the free inode in the dirty st123
 ate is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c EDCINVAL, if the data cluster header is inconsistent
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soCleanDataCluster (uint32_t nInode, uint32_t nLClust)
{
  soColorProbe (415, "07;31", "soCleanDataCluster (%"PRIu32", %"PRIu32")\n", nInode, nLClust);
	SOSuperBlock* p_sb;
	SOInode p_inode;
	uint32_t stat;
	//Obter informação do SuperBloco
	if((stat=soLoadSuperBlock())!=0){
		return stat;
	}
	//Ponteiro para superbloco
	p_sb=soGetSuperBlock();

	
	//Verificar se nInode e nLClust são validos
	if(nInode>=p_sb->iTotal || nInode<=0 || nLClust>=p_sb->dZoneTotal){
		return -EINVAL;
	}

	/*if((stat=soConvertRefInT(nInode,&nblk,&offset))!=0){
		return stat;
	}

	if((stat=soLoadBlockInT(nblk))!=0){
		return stat;
	}
	// obtençao de ponteiro para o bloco onde se encontra o Inode
	p_inode=soGetBlockInT();
	*/

	if((stat=soReadInode(&p_inode,nInode,FDIN))!=0)
	{
		return stat;
	}
	//Se o inode livre no estado sujo for inconsistente
	if((soQCheckFDInode(p_sb,&p_inode))!=0){
		return -EFDININVAL;
	}

	if(((stat=soQCheckFDInode(p_sb,&p_inode)))!=0){
		return stat;
	}

	SODataClust* ref_clust;
	// numero total de clusters que compoem o Inode
	int total_clust=p_inode.cluCount;
	// contador que indica quantos clusters do Inode ja foram encontrados
    int count = 0;
    int i = 0,c = 0;


    // array de referenciação directa
	for(i=0;i<N_DIRECT;i++)
	{
		c=p_inode.d[i];

		if(c != NULL_CLUSTER)
		{
			if(c == nLClust)
			{
				// limpeza o cluster de dados atraves da função HandleFileCluster
				if((stat=soHandleFileCluster(nInode,i,CLEAN,NULL))!=0)
				{
					return stat;
				}
				return 0;
			}
			// se cluster exsite ( != NULL_CLUSTER ) e nao e o que pretendemos entao contador incrementa			
			else
			{
				count++;
			}
		} 	
		// percorremos enconcontramos nClusters = ao total de CLusters do inode sem que nLClust tenha aparecido
		if(count==total_clust)
		{
			 return -EDCINVAL;
		}			
	}

	int k = 0;
	//Carregar cluster i1 de referência simplesmente indirecta para memória interna
	if((stat=soLoadDirRefClust(p_sb->dZoneStart+((p_inode.i1)*BLOCKS_PER_CLUSTER)))!=0)
	{
		return stat;
	}
	// ponteiro para custer que esteja na memoria interna
	ref_clust=soGetDirRefClust();
	// Cluster I1 de referenciação simplesmente indirecta
	if(p_inode.i1 == nLClust)
	{
		for (k = 0; k < RPC; k++)
		{
			if(ref_clust->info.ref[k]!=NULL_CLUSTER)
			{
				if((stat=soHandleFileCluster(nInode,N_DIRECT+k,CLEAN,NULL))!=0)
				{
					return stat;
				}
			}
		}
		return 0;
	}
	else
	{
		for(k=0;k<RPC;k++)
		{
			if(ref_clust->info.ref[k]!=NULL_CLUSTER)
			{
				if(ref_clust->info.ref[k]==nLClust)
				{
					if((stat=soHandleFileCluster(nInode,N_DIRECT+k,CLEAN,NULL))!=0)
					{
						return stat;
					}
					return 0;
				}
				else 
				{
					count++;
				}
			}
			if(count==total_clust)
			{
				return -EDCINVAL;
			}
		}		
	}
	// carregar cluster de referencias I2 para a memoria interna
	if((stat=soLoadSngIndRefClust(p_sb->dZoneStart+(p_inode.i2*BLOCKS_PER_CLUSTER)))!=0)
	{
		return stat;
	}
	ref_clust = soGetSngIndRefClust();
	if(p_inode.i2 == nLClust)
	{
		for (k=0;k<RPC;k++)
		{
			if((stat=soLoadSngIndRefClust(p_sb->dZoneStart+(p_inode.i2*BLOCKS_PER_CLUSTER)))!=0)
			{
				return stat;
			}
			ref_clust=soGetSngIndRefClust();
			if(ref_clust->info.ref[k] != NULL_CLUSTER)
			{
				if((stat=soLoadDirRefClust(p_sb->dZoneStart+((ref_clust->info.ref[k])*BLOCKS_PER_CLUSTER)))!=0)
				{
					return stat;
				}
				// ponteiro para custer que esteja na memoria interna
				ref_clust=soGetDirRefClust();

				for (i=0;i<RPC;i++)
				{
					if(ref_clust->info.ref[i]!=NULL_CLUSTER)
					{
						if((stat=soHandleFileCluster(nInode,N_DIRECT+(RPC*(k+1))+i,CLEAN,NULL))!=0)
						{
							return stat;
						}
					}
				}
			}
		}
		return 0;
	}
	else
	{
	// percorrer todas as referencias possiveis no cluster I2
		for(k=0;k<RPC;k++)
		{
			// carregar cluster de referencias I2 para a memoria interna
			if((stat=soLoadSngIndRefClust(p_sb->dZoneStart+(p_inode.i2*BLOCKS_PER_CLUSTER)))!=0)
			{
				return stat;
			}
			ref_clust = soGetSngIndRefClust();
			if(ref_clust->info.ref[k]!=NULL_CLUSTER)
			{
				/* se cluster i1[k] == nLCluster temos que carregar i1[k] limpar e libertar todos os 
				clusters de dados por ele referenciados e depois por fim libertar o cluster
				de referencias i1[k]*/
				if(ref_clust->info.ref[k]==nLClust)
				{
					if((stat=soLoadDirRefClust(p_sb->dZoneStart+((ref_clust->info.ref[k])*BLOCKS_PER_CLUSTER)))!=0)
					{
						return stat;
					}
					ref_clust=soGetDirRefClust();
					/*percorrer todos e limpar todos os cluster de dados referenciados pelo cluster
					de referencias i1[k]*/
					for (i = 0; i < RPC; i++)
					{
						if(ref_clust->info.ref[i]!=NULL_CLUSTER)
						{
							if((stat=soHandleFileCluster(nInode,N_DIRECT+((k+1)*RPC)+i,CLEAN,NULL))!=0)
							{
								return stat;
							}
						}
					}
					return 0;
				}
				/* caso cluster de referencias i1[k] nao seja o pretendido (nLCluster), 
				temos entao que o carregar, e procurar em todos os cluster de dados referenciados 
				por este mesmo cluster de referencias */
				else
				{
					if((stat=soLoadDirRefClust(p_sb->dZoneStart+((ref_clust->info.ref[k])*BLOCKS_PER_CLUSTER)))!=0)
					{
						return stat;
					}
					ref_clust=soGetDirRefClust();

					for ( i = 0; i<RPC; i++)
					{

						if (ref_clust->info.ref[i] != NULL_CLUSTER)
						{
							if (ref_clust->info.ref[i] == nLClust)
							{
								if((stat=soHandleFileCluster(nInode,N_DIRECT+((k+1)*RPC)+i,CLEAN,NULL))!=0)
								{
									return stat;
								}
								return 0;
							}
							else
							{
								count++;
							}
						}
						/* foram encontrados tantos clusters de dados quandos os 
						clusters de dados do inode sem que encontrasse nLClust*/ 
						if (count == total_clust)
						{
							return -EDCINVAL;
						}
					}	
					count++;
				}
				if (count == total_clust)
				{
					return -EDCINVAL;
				}
			}
		}
	}
 	return 0;
}
