/**
 *  \file soFreeInode.c (implementation file)
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
#include "sofs_buffercache.h"
#include "sofs_superblock.h"
#include "sofs_inode.h"
#include "sofs_datacluster.h"
#include "sofs_basicoper.h"
#include "sofs_basicconsist.h"

/**
 *  \brief Free the referenced inode.
 *
 *  The inode must be in use, belong to one of the legal file types and have no directory entries associated with it
 *  (refcount = 0).
 *  The inode is marked free in the dirty state and inserted in the list of free inodes.
 *
 *  Notice that the inode 0, supposed to belong to the file system root directory, can not be freed.
 *
 *  The only affected fields are:
 *     \li the free flag of mode field, which is set
 *     \li the <em>time of last file modification</em> and <em>time of last file access</em> fields, which change their
 *         meaning: they are replaced by the <em>prev</em> and <em>next</em> pointers in the double-linked list of free
 *         inodes.
 * *
 *  \param nInode number of the inode to be freed
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>inode number</em> is out of range
 *  \return -\c EIUININVAL, if the inode in use is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c EDCINVAL, if the data cluster header is inconsistent
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soFreeInode (uint32_t nInode)
{
	soColorProbe (612, "07;31", "soFreeInode (%"PRIu32")\n", nInode);

	SOInode* p_inode;
	SOSuperBlock* p_sb;
	uint32_t nBlk, offset;
	int stat;

	/*obter informaçao superbloco*/
	if((stat = soLoadSuperBlock()) != 0) return stat;
	p_sb = soGetSuperBlock();

	/*verificar se o iNode não é o 0 que nao pode ser colocado a free, ou se está dentro dos parametros, neste caso se o nInode é menor que o p_sb->iTotal*/
	if(nInode >= p_sb->iTotal || nInode <= 0) return -EINVAL;

	/*leitura do inode a ser libertado*/

	if((stat = soConvertRefInT(nInode, &nBlk, &offset)) != 0) return stat;
	if((stat = soLoadBlockInT(nBlk)) != 0) return stat;

	p_inode = soGetBlockInT();

	/*inode deverá estar em uso*/

	if((stat = soQCheckInodeIU(p_sb, &p_inode[offset])) != 0) return stat;

	p_inode[offset].mode = INODE_FREE;

	/*se a lista de nos free estiver vazia, entao o prev e next do inode sao null, e este passa a ser o head e tail*/
	if(p_sb->iFree==0){
		p_inode[offset].vD2.prev = p_inode[offset].vD1.next = NULL_INODE;
		p_sb->iHead = p_sb->iTail = nInode; 


	}else{				/*caso não esteja livre, então o inode passa a ser o tail, e o seu next é NULL */
		p_inode[offset].vD2.prev = p_sb->iTail;
		p_inode[offset].vD1.next = NULL_INODE;
		p_sb->iTail = nInode;

	}

	p_sb->iFree +=1;

	/*armazenar informaçao do bloco que contem o inode*/
	if((stat = soStoreBlockInT()) != 0) return stat;

	/*armazenar informaçao do superbloco*/
	if((stat = soStoreSuperBlock()) != 0) return stat;

	/*para o penultimo inode é necessario colocar o seu next com o novo inode libertado*/
	if(p_sb->iFree > 1){
		if((stat = soConvertRefInT(p_inode[offset].vD2.prev, &nBlk, &offset)) != 0) return stat;
		if((stat = soLoadBlockInT(nBlk)) != 0) return stat;

		p_inode = soGetBlockInT();
		p_inode[offset].vD1.next = nInode;

		if((stat = soStoreBlockInT()) != 0) return stat;
	}
	return 0;
}
