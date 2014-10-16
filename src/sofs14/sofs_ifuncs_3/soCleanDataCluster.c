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
	SODataClust c;
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

			if(stat=soConvertRefInt(nInode,&nblk,&offset)!=0){
				return stat;
			}

			if(stat=(soLoadBlockInT(nblk))!=0){
				return stat;
			}

		p_inode=soGetBlockInT();

	//Se o inode livre no estado sujo for inconsistente
	if((soQCheckFDInode(p_sb,&p_inode[offset]))!=0){
		return -EFDININVAL;

				}

	if((soQCheckDirCont(p_sb,&p_inode[offset]))!=0){

					return -ELDCININVAL;
				}

	SODataClust* ref_clust;
	int total_clust=p_inode.size/CLUSTER_SIZE;
    int count = 0;
    
	for(i=0;i<N_Direct;i++){
		c=p_inode[offset].d[i];

		if(c.stat != NULL_INODE)
		{
			if(c.stat == nLClust)
			{
				soFreeDataCluster(nLClust);
				p_inode[offset].d[i]=NULL_CLUSTER;
			}
			
			else
			{
				count++;
			}
		} 	

			if(count==total_clust){
					return /*Total de clusTERS do inode atingido*/
			}			

	}
		boolean n2=false;
		boolean n1=false;
		boolean done = false;
		int k,trash;
		//Carregar clusters de referência interna para memória
		if((stat=soLoadSngIndRefClust(p_inode[offset].i1))!=0){
		
		ref_clust=soGetSngIndRefClust();

		for(k=0;k<RPC;k++){

			if(ref_clust.info.ref[k]!=NULL_CLUSTER){
				
				
				if(ref_clust.info.ref[k]==nLClust){
					count++;
					trash=soFreeDataCluster(nLClust);
					ref_clust.info.ref[k]=NULL_CLUSTER;
					done = true;
					if ( n1 == true){
						if((stat=soStoreSngIndRefClust())!=0){
							return stat;
						}
						return 0;
					}

					}

				else {
					n1 = true;
					count++;
				}

			}

			if(count==total_clust && n1==true){
				if((stat=soStoreSngIndRefClust())!=0){
					return stat;
				}
				return /*Falar prof*/
			}



		}

		if(n1==false){

				trash=soFreeDataCluster(p_inode[offset].i1);
				p_inode[offset].i1=NULL_CLUSTER;
				if ( done == true){
					return 0;
				}
		}		
		else{
				if((stat=soStoreSngIndRefClust())!=0){
					return stat;
				}
				if ( done == true){
					return 0;
				}

			}


			if((stat=soLoadSngIndRefClust(p_inode[offset].i2)!=0){

					return stat;
			}


			for(k=0;k<RPC;k++){

				if(ref_clust.info.ref[k]!=NULL_CLUSTER){

					if(ref_clust.info.ref[k]==nLClust){
							count++;
							if((stat=soLoadSngIndRefClust(ref_clust.info.ref[k]))!=0){
								return stat;
							}
							for (int i = 0; i < RPC; i++)
							{
								if(ref_clust.info.ref[i]!=NULL_CLUSTER){
									count++;
									trash=soFreeDataCluster(ref_clust.info.ref[i]);
									}

							}

							if((stat=soLoadSngIndRefClust(p_inode[offset].i2))!=0){
									return stat;
					}
						trash=soFreeDataCluster(ref_clust.info.ref[k]);
						ref_clust.info.ref[k]=NULL_CLUSTER;
						done = true;
						if((stat=soStoreSngIndRefClust())!=0){
					return stat;
				}
					}
					else{
						if((stat=soLoadSngIndRefClust(ref_clust.info.ref[k]))!=0){
								return stat;
							}
						n1 = false;
						
						for ( i = 0; i<RPC; i++){

							if (ref_clust.info.ref[i] != NULL_CLUSTER){
								if (ref_clust.info.ref[i] == nLClust){
									count++;
									/*if((stat=soStoreSngIndRefClust())!=0){
										return stat;
									}*/
									trash = soFreeDataCluster(nLClust);
									ref_clust.info.ref[i] = NULL_CLUSTER;
									if (n1==true){
										if((stat=soStoreSngIndRefClust())!=0){
											return stat;
										}
										return 0;
									}
								}
								else{
									count++;
									n1 = true;
								}
							}
							if (count == total_clust && n1 == true){
								if((stat=soStoreSngIndRefClust())!=0){
									return stat;
								}
								return 0;
							}
							if (done == true && n1 == true){
								return 0;
							}
						}	
						if ( n1 == true){
							n2 = true;
						}
						else{
							if((stat=soLoadSngIndRefClust(p_inode[offset].i2))!=0){
									return stat;
							}
							trash = soFreeDataCluster(ref_clust.info.ref[k]);
							ref_clust.info.ref[k] = NULL_CLUSTER;
							if((stat=soStoreSngIndRefClust())!=0){
										return stat;
							}
							if (n2 == true){
								return 0;
							}
						}
						count++;

					}


				}
				if (n2==true && done == true){
					return 0;
				}
				if(n2==true && count==total_clust){
					return /* Falar prof*/
				}
			}
			if (n2 == false){
				trash = soFreeDataCluster(p_inode[offset].i2);
				p_inode[offset].i2 = NULL_CLUSTER;
			}

	if ((stat = soStoreBlockInT())!= 0){
		return stat;
	}
	if ((stat = soStoreSuperBlock())!=0){
		return stat;
	}
  
  return 0;
}
