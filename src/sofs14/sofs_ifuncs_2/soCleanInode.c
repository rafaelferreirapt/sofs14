/**
 *  \file soCleanInode.c (implementation file)
 *
 *  \author
 */

#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#include "sofs_probe.h"
#include "sofs_superblock.h"
#include "sofs_inode.h"
#include "sofs_basicoper.h"
#include "sofs_basicconsist.h"
/* #define  CLEAN_INODE */
#ifdef CLEAN_INODE
#include "sofs_ifuncs_3.h"
#endif

/** \brief inode in use status */
#define IUIN  0
/** \brief free inode in dirty state status */
#define FDIN  1

/* allusion to internal function */

int soReadInode (SOInode *p_inode, uint32_t nInode, uint32_t status);

/**
 *  \brief Clean an inode.
 *
 *  The inode must be free in the dirty state.
 *  The inode is supposed to be associated to a file, a directory, or a symbolic link which was previously deleted.
 *
 *  This function cleans the list of data cluster references.
 *
 *  Notice that the inode 0, supposed to belong to the file system root directory, can not be cleaned.
 *
 *  \param nInode number of the inode
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>inode number</em> is out of range
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

int soCleanInode (uint32_t nInode)
{
	soColorProbe (513, "07;31", "soCleanInode (%"PRIu32")\n", nInode);

	SOSuperBlock* p_sb;
	SOInode* p_inode;
	int stat;
	uint32_t nBlk,offset;

	//Obter informação so SuperBloco
	if((stat=soLoadSuperBlock())!=0){
		return stat;
	}

	p_sb=soGetSuperBlock(); //Ponteiro para superbloco

	//Verificar se nInode é valido
	if(nInode >= p_sb->iTotal || nInode <= 0){
		return -EINVAL;
	}

	if((stat=soConvertRefInT(nInode,&nBlk,&offset))!=0){
		return stat;
	} 	
	//Obter informação da tabela de Inodes	
	if((stat=soLoadBlockInT(nBlk))!=0){
		return stat;
	}
	p_inode=soGetBlockInT(); //Ponteiro para tabela de Inodes

	//Se o inode livre no estado sujo for inconsistente
	if((stat = soQCheckFDInode(p_sb,&p_inode[offset]))!=0){
		return stat;
	}

	soReadInode(p_inode,nInode,stat);

	if((stat = soStoreBlockInT()) != 0){ //Inserir informação da tabela de Inodes no dispositivo
		return stat;
	}

	if((stat=soStoreSuperBlock())!=0){ //Inserir informação no dispositivo

		return stat;

	}

	return 0;
}
