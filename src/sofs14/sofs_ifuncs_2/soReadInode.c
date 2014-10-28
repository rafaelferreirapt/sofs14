/**
 *  \file soReadInode.c (implementation file)
 *
 *  \author
 */

/* #define CLEAN_INODE */

#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h> 

#include "sofs_probe.h"
#include "sofs_superblock.h"
#include "sofs_inode.h"
#include "sofs_basicoper.h"
#include "sofs_basicconsist.h"
//#ifdef CLEAN_INODE
#include "sofs_ifuncs_3.h"
//#endif

/** \brief inode in use status */
#define IUIN  0
/** \brief free inode in dirty state status */
#define FDIN  1

/**
 *  \brief Read specific inode data from the table of inodes.
 *
 *  The inode may be either in use and belong to one of the legal file types or be free in the dirty state.
 *  Upon reading, the <em>time of last file access</em> field is set to current time, if the inode is in use.
 *
 *  \param p_inode pointer to the buffer where inode data must be read into
 *  \param nInode number of the inode to be read from
 *  \param status inode status (in use / free in the dirty state)
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>buffer pointer</em> is \c NULL or the <em>inode number</em> is out of range or the
 *                      inode status is invalid
 *  \return -\c EIUININVAL, if the inode in use is inconsistent
 *  \return -\c EFDININVAL, if the free inode in the dirty state is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c EDCINVAL, if the data cluster header is inconsistent
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soReadInode (SOInode *p_inode, uint32_t nInode, uint32_t status){
	soColorProbe (511, "07;31", "soReadInode (%p, %"PRIu32", %"PRIu32")\n", p_inode, nInode, status);

	/* verificar se o ponteiro que aponta para o buffer onde os dados do inode devem ser lidos não é nulo e se o status está em uso ou free in dirty state */
  	if(p_inode == NULL || (status != IUIN && status != FDIN)){
    	return -EINVAL;
  	} 

	uint32_t nBlk, offset;
	int stat;
	SOInode *p_tmp_inode;
	SOSuperBlock *p_sb;

	if((stat = soLoadSuperBlock()) != 0){
		return stat;
	}

	p_sb = soGetSuperBlock();

	if((stat = soQCheckSuperBlock(p_sb)) != 0){
		return stat;
	}

	//Verifica a consitência da tabela de nós-i
    if((stat = soQCheckInT(p_sb)) != 0){
    	return stat;
    }

  	/* nº do inode que deve ser lido */
  	if(nInode < 0 || nInode >= p_sb->iTotal){
    	return -EINVAL;
  	}

  	if(nInode < 0 || nInode > p_sb->iTotal || p_inode == NULL || (status != IUIN && status != FDIN)){
		return -EINVAL;
	}

	/*obtençao do numero do bloco e respectivo offset para o nInode pretendido*/
	if ((stat = soConvertRefInT(nInode, &nBlk, &offset)) != 0){
		return stat;
	}

	/*copia do bloco que contem o iNode para a area de armazenamento interno*/
	if((stat = soLoadBlockInT(nBlk)) != 0){
		return stat;
	}
	/*ponteiro para a area de armazenamento interno*/
	p_tmp_inode = soGetBlockInT();

	
	if(status == IUIN){
		if((stat = soQCheckInodeIU(p_sb, &p_tmp_inode[offset]))){
			return stat;
		}
		p_tmp_inode[offset].vD1.aTime = time(NULL);

	}else if(status == FDIN){
		if((stat = soQCheckFDInode(p_sb, &p_tmp_inode[offset]))){
			return stat;
		}
	}
	
	memcpy(p_inode, &p_tmp_inode[offset], sizeof(SOInode));  	

	if((stat = soStoreBlockInT()) != 0){
	 	return stat;
	}

	if((stat = soStoreSuperBlock()) != 0){
    	return stat;
	}

	return 0;
}
