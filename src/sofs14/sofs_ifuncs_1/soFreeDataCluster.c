/**
 *  \file soFreeDataCluster.c (implementation file)
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
#include "../sofs_superblock.h"
#include "../sofs_inode.h"
#include "../sofs_datacluster.h"
#include "../sofs_basicoper.h"
#include "../sofs_basicconsist.h"

/* Allusion to internal function */

int soDeplete (SOSuperBlock *p_sb);

/**
 *  \brief Free the referenced data cluster.
 *
 *  The cluster is inserted into the insertion cache of free data cluster references. If the cache is full, it has to be
 *  depleted before the insertion may take place. The data cluster should be put in the dirty state (the <tt>stat</tt>
 *  of the header should remain as it is), the other fields of the header, <tt>prev</tt> and <tt>next</tt>, should be
 *  put to NULL_CLUSTER. The only consistency check to carry out at this stage is to check if the data cluster was
 *  allocated.
 *
 *  Notice that the first data cluster, supposed to belong to the file system root directory, can never be freed.
 *
 *  \param nClust logical number of the data cluster
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, the <em>data cluster number</em> is out of range or the data cluster is not allocated
 *  \return -\c EDCNALINVAL, if the data cluster has not been previously allocated
 *  \return -\c EDCINVAL, if the data cluster header is inconsistent
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soFreeDataCluster (uint32_t nClust)
{
  SOSuperBlock* p_sb; //Criação de um ponteiro para o superbloco
  uint32_t p_stat; //variavel para estado do cluster de dados
  int error; //Variavel para verificações de consistência

  soColorProbe (614, "07;33", "soFreeDataCluster (%"PRIu32")\n", nClust);

	//Obter o bloco de superbloco
	if((error=soLoadSuperBlock())!=0){

		return error;

		}
	//Ponteiro para superbloco
	p_sb=soGetSuperBlock();

	//Verificar se nClust esta dentro da gama!!
	if((soQCheckStatDC(p_sb,nClust,&p_stat))!=0){

		return -EINVAL;

			}
	//Verificar se o cluster esta ou não alocado...
	if(p_stat!=ALLOC_CLT){

		return -EDCNALINVAL;
			}

	// Verificar a consistência do header do cluster de dados
	if((soQCheckDZ(p_sb))!=0){

		return -EDCINVAL;
			}



	p_sb->dHead=p_sb->dTail=NULL_CLUSTER; //Inicialização do head e tail

	//the insertion cache is full, deplete it*/

	if(p_sb->dZoneInsert.cacheIdx==DZONE_CACHE_SIZE){

	deplete(p_sb);
		}


	p_sb->dZoneInsert.cache[p_sb->dZoneInsert.cacheIdx]=nClust;		//Um cluster livre é inserido na posição cacheIdx
	p_sb->dZoneInsert.cacheIdx+=1;			//Incrementa para apontar para a proxima posição onde pode ser inserido um cluster
	p_sb->dZoneFree+=1;				//Incrementa nº de clusters livres!!




 	if((error==soQCheckSuperBlock(p_sb))!=0){ 	//Verificar consistência do superbloco

		return error;

		}


	if((error==soStoreSuperBlock())!=0){		//Inserir informação no dispositivo

		return error;

			}

  return 0;
}

/**
 *  \brief Deplete the insertion cache of free data cluster references.
 *
 *  \param p_sb pointer to a buffer where the superblock data is stored
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soDeplete (SOSuperBlock *p_sb)
{
	int cachePos, stat;
	SODataClust dclust;
	uint32_t NumClu;

	if(p_sb->dTail != NULL_CLUSTER){

		/* Cálculo do physical number do cluster apontado por dtail */
		NumClu = p_sb->dZoneStart + p_sb->dTail * BLOCKS_PER_CLUSTER;

	    /* Verificação de erros e leitura do cluster */
        if((erro = soReadCacheCluster(NumClu, &dclust)) != 0) return erro;

        dclust.next = p_sb->dZoneInsert.cache[0];

        /* Verificação de erros e escrita do cluster */
        if((erro = soWriteCacheCluster(NumClu, &dclust)) != 0) return erro;

    }

	for(cPos = 0; cPos < DZONE_CACHE_SIZE; cPos++) {

		/* Cálculo do physical number do cluster apontado */
		NumClu = p_sb->dZoneStart + p_sb->dZoneInsert.cache[cPos] * BLOCKS_PER_CLUSTER;

	  	/* Verificação de erros e leitura do cluster */
        if((erro = soReadCacheCluster(NumClu, &dclust)) != 0) return erro;

        if(cPos == 0){
        	dclust.prev = p_sb->dTail;
        }
        else{
        	dclust.prev = p_sb->dZoneInsert.cache[cPos - 1];
        }

        if(cPos != (DZONE_CACHE_SIZE - 1)){
        	dclust.next = p_sb->dZoneInsert.cache[cPos + 1];
        }

        else{
        	dclust.next = NULL_CLUSTER;
        }

	    /* Verificação de erros e escrita do cluster */
        if((erro = soWriteCacheCluster(NumClu, &dclust)) != 0) return erro;
	}

	/* Coloca-se o dtail a apontar para o último cluster da cache */
	p_sb->dTail = p_sb->dZoneInsert.cache[DZONE_CACHE_SIZE - 1];

	if(dHead == NULL_CLUSTER) {
	    p_sb->dHead = p_sb->dZoneInsert.cache[0];
	}

	/* Limpeza da cache de inserção */
	for(cPos = 0; cPos < DZONE_CACHE_SIZE; cPos++){
	    p_sb->dZoneInsert.cache[cPos] = NULL_CLUSTER;
	}

	p_sb->dZoneInsert.cacheIdx = 0;

	return 0;
}
