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
	SOInode* p_inode;
	uint32_t stat,nblk,offset;
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

	if(stat=soConvertRefInT(nInode,&nblk,&offset)!=0){
		return stat;
	}

	if(stat=(soLoadBlockInT(nblk))!=0){
		return stat;
	}
	// obtençao de ponteiro para o bloco onde se encontra o Inode
	p_inode=soGetBlockInT();

	//Se o inode livre no estado sujo for inconsistente
	if((soQCheckFDInode(p_sb,&p_inode[offset]))!=0){
		return -EFDININVAL;

				}

	if((stat=soQCheckFDInode(p_sb,&p_inode[offset]))!=0){

					return stat;
				}

	SODataClust* ref_clust;
	// numero total de clusters que compoem o Inode
	int total_clust=p_inode[offset].size/CLUSTER_SIZE;
	// contador que indica quantos clusters do Inode ja foram encontrados
    int count = 0;
    int i,c;


    // array de referenciação directa
	for(i=0;i<N_DIRECT;i++)
	{
		c=p_inode[offset].d[i];

		if(c != NULL_CLUSTER)
		{
			if(c == nLClust)
			{
				// limpeza o cluster de dados atraves da função HandleFileCluster
				if((stat=soHandleFileCluster(nInode,i,FREE_CLEAN,NULL))!=0)
				{
					return stat;
				}
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
			 perror("Cluster Not Found");
		}			
	}
	bool n2=false;
	bool n1=false;
	bool done = false;
	int k,trash;
	//Carregar cluster i1 de referência simplesmente indirecta para memória interna
	
	if((stat=soLoadSngIndRefClust(p_inode[offset].i1))!=0){
		return stat;
	}
	
	perror("Depois de I1");
	// ponteiro para custer que esteja na memoria interna
	ref_clust=soGetSngIndRefClust();
	// Cluster I1 de referenciação simplesmente indirecta
	for(k=0;k<RPC;k++)
	{

		if(ref_clust->info.ref[k]!=NULL_CLUSTER)
		{
				
			if(ref_clust->info.ref[k]==nLClust)
			{
				count++;
				if((stat=soHandleFileCluster(nInode,N_DIRECT+k,FREE_CLEAN,NULL))!=0)
				{
					return stat;
				}
				done = true;
				//caso i1 nao fique vazia apos se remover nLClust, funçao termina aqui
				if ( n1 == true)
				{
					if((stat=soStoreSngIndRefClust())!=0)
					{
						return stat;
					}
					// armazenar de volta informaçao do Inode
					if ((stat = soStoreBlockInT())!= 0)
					{
						return stat;
					}
					// armazenar de volta a informação do superbloco
					if ((stat = soStoreSuperBlock())!=0)
					{
						return stat;
					}
					return 0;
				}

			}
			/* caso cluster encontrado nao seja o que pretendemos incrementamos
			contador e sinalizamos I1 como nao vazio ( n1 = true) */
			else 
			{
				n1 = true;
				count++;
			}

		}

		if(count==total_clust && n1==true)
		{
			if((stat=soStoreSngIndRefClust())!=0)
			{
				return stat;
			}
			perror("Cluster Not Found");
		}
	}
	// caso o cluster de referencia I1 tenha ficado livre/vazio
	if(n1==false)
	{
		trash=soFreeDataCluster(p_inode[offset].i1);
		p_inode[offset].i1=NULL_CLUSTER;
		if ( done == true)
		{
			if((stat=soStoreSngIndRefClust())!=0)
			{
				return stat;
			}
			// armazenar de volta informaçao do Inode
			if ((stat = soStoreBlockInT())!= 0)
			{
				return stat;
			}
			// armazenar de volta a informação do superbloco
			if ((stat = soStoreSuperBlock())!=0)
			{
				return stat;
			}
			return 0;
		}
	}		
	else
	{
		if((stat=soStoreSngIndRefClust())!=0)
		{
			return stat;
		}
		// cluster nao ficou vazio mas nLCluster ja foi limpo
		if ( done == true)
		{
			// armazenar de volta informaçao do Inode
			if ((stat = soStoreBlockInT())!= 0)
			{
				return stat;
			}
			// armazenar de volta a informação do superbloco
			if ((stat = soStoreSuperBlock())!=0)
			{
				return stat;
			}
			return 0;
		}

	}
	// carregar cluster de referencias I2 para a memoria interna
	if((stat=soLoadSngIndRefClust(p_inode[offset].i2))!=0)
	{
		return stat;
	}
	// percorrer todas as referencias possiveis no cluster I2
	for(k=0;k<RPC;k++)
	{

		if(ref_clust->info.ref[k]!=NULL_CLUSTER)
		{
			/* se cluster i1[k] == nLCluster temos que carregar i1[k] limpar e libertar todos os 
			clusters de dados por ele referenciados e depois por fim libertar o cluster
			de referencias i1[k]*/
			if(ref_clust->info.ref[k]==nLClust)
			{
				count++;
				if((stat=soLoadSngIndRefClust(ref_clust->info.ref[k]))!=0)
				{
					return stat;
				}
				/*percorrer todos e limpar todos os cluster de dados referenciados pelo cluster
				de referencias i1[k]*/
				for (i = 0; i < RPC; i++)
				{
					if(ref_clust->info.ref[i]!=NULL_CLUSTER)
					{
						count++;
						if((stat=soHandleFileCluster(nInode,N_DIRECT+((k+1)*RPC)+i,FREE_CLEAN,NULL))!=0)
						{
							return stat;
						}
					}
				}
				/* carregar novamente o cluster de referencias i2 e libertar
				o recem limpo cluster de referencias i1[k] */
				if((stat=soLoadSngIndRefClust(p_inode[offset].i2))!=0)
				{
					return stat;
				}
				trash=soFreeDataCluster(ref_clust->info.ref[k]);
				ref_clust->info.ref[k]=NULL_CLUSTER;
				done = true;
				if((stat=soStoreSngIndRefClust())!=0)
				{
					return stat;
				}
			}
			/* caso cluster de referencias i1[k] nao seja o pretendido (nLCluster), 
			temos entao que o carregar, e procurar em todos os cluster de dados referenciados 
			por este mesmo cluster de referencias */
			else
			{
				if((stat=soLoadSngIndRefClust(ref_clust->info.ref[k]))!=0)
				{
					return stat;
				}
				/* n1 = false, partimos do principio que o cluster de 
				dados esta vazio */
				n1 = false;

				for ( i = 0; i<RPC; i++)
				{

					if (ref_clust->info.ref[i] != NULL_CLUSTER)
					{
						if (ref_clust->info.ref[i] == nLClust)
						{
							count++;
									/*if((stat=soStoreSngIndRefClust())!=0){
										return stat;
									}*/
							if((stat=soHandleFileCluster(nInode,N_DIRECT+(k+1)*RPC+i,FREE_CLEAN,NULL))!=0)
							{
								return stat;
							}			
							/* claso cluster apagado e cluster de 
							referencias nao vazio, funçao termina */
							if (n1==true)
							{
								if((stat=soStoreSngIndRefClust())!=0)
								{
									return stat;
								}
								if ((stat = soStoreBlockInT())!= 0)
								{
									return stat;
								}
								// armazenar de volta a informação do superbloco
								if ((stat = soStoreSuperBlock())!=0)
								{
									return stat;
								}
								return 0;
							}
						}
						else
						{
							count++;
							n1 = true;
						}
					}
					/* foram encontrados tantos clusters de dados quandos os 
					clusters de dados do inode sem que encontrasse nLClust*/ 
					if (count == total_clust && done == false)
					{
						if((stat=soStoreSngIndRefClust())!=0)
						{
							return stat;
						}
						if ((stat = soStoreBlockInT())!= 0)
						{
							return stat;
						}
						// armazenar de volta a informação do superbloco
						if ((stat = soStoreSuperBlock())!=0)
						{
							return stat;
						}
						perror("Cluster Not Found");
					}
					/* se nLClust limpo e i1[k] nao vazio, funçao terminada*/
					if (done == true && n1 == true)
					{
						if ((stat = soStoreBlockInT())!= 0)
						{
							return stat;
						}
						// armazenar de volta a informação do superbloco
						if ((stat = soStoreSuperBlock())!=0)
						{
							return stat;
						}
						return 0;
					}
				}	
				if ( n1 == true)
				{
					n2 = true;
				}
				/* caso ao remover mos o nLClust i1[k] fique vazio,
				temos que o libertar */
				else
				{
					if((stat=soLoadSngIndRefClust(p_inode[offset].i2))!=0)
					{
						return stat;
					}
					trash = soFreeDataCluster(ref_clust->info.ref[k]);
					ref_clust->info.ref[k] = NULL_CLUSTER;
					if((stat=soStoreSngIndRefClust())!=0)
					{
						return stat;
					}
					/* apos i1[k] removido, se i2 nao ficar estiver vazio ( n2 != false)
					a funçao termina */
					if (n2 == true)
					{
						if ((stat = soStoreBlockInT())!= 0)
						{
							return stat;
						}
						// armazenar de volta a informação do superbloco
						if ((stat = soStoreSuperBlock())!=0)
						{
							return stat;
						}
						return 0;
					}
				}
				count++;
			}
		}
		/* se apos percorrar todas posiçoes possiveis, i2
		nao ficou vazio e nLClust foi limpo, a funçao termina */
		if (n2==true && done == true)
		{
			if ((stat = soStoreBlockInT())!= 0)
			{
				return stat;
			}
			// armazenar de volta a informação do superbloco
			if ((stat = soStoreSuperBlock())!=0)
			{
				return stat;
			}
			return 0;
		}
		/* caso i2 nao vazio e count = total de clusters de dados do Inode,
		nLClust nao encontrado */
		if(n2==true && count==total_clust)
		{
			if ((stat = soStoreBlockInT())!= 0)
			{
				return stat;
			}
			// armazenar de volta a informação do superbloco
			if ((stat = soStoreSuperBlock())!=0)
			{
				return stat;
			}
			 perror("Cluster Not Found");
		}
	}if ((stat = soStoreBlockInT())!= 0)
	{
		return stat;
	}
	// armazenar de volta a informação do superbloco
	if ((stat = soStoreSuperBlock())!=0)
	{
		return stat;
	}
	// caso cluster de referencias I2 tenha ficado livre/vazio temos que o remover/limpar
	if (n2 == false)
	{
		trash = soFreeDataCluster(p_inode[offset].i2);
		p_inode[offset].i2 = NULL_CLUSTER;
	}
	// armazenar de volta informaçao do Inode
	if ((stat = soStoreBlockInT())!= 0)
	{
		return stat;
	}
	// armazenar de volta a informação do superbloco
	if ((stat = soStoreSuperBlock())!=0)
	{
		return stat;
	}
  return 0;
}
