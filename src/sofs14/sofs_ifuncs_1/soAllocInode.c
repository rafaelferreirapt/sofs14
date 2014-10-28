/**
 *  \file soAllocInode.c (implementation file)
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
#include "sofs_ifuncs_2.h"

/**
 *  \brief Allocate a free inode.
 *
 *  The inode is retrieved from the list of free inodes, marked in use, associated to the legal file type passed as
 *  a parameter and generally initialized. It must be free and if is free in the dirty state, it has to be cleaned
 *  first.
 *
 *  Upon initialization, the new inode has:
 *     \li the field mode set to the given type, while the free flag and the permissions are reset
 *     \li the owner and group fields set to current userid and groupid
 *     \li the <em>prev</em> and <em>next</em> fields, pointers in the double-linked list of free inodes, change their
 *         meaning: they are replaced by the <em>time of last file modification</em> and <em>time of last file
 *         access</em> which are set to current time
 *     \li the reference fields set to NULL_CLUSTER
 *     \li all other fields reset.

 *  \param type the inode type (it must represent either a file, or a directory, or a symbolic link)
 *  \param p_nInode pointer to the location where the number of the just allocated inode is to be stored
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>type</em> is illegal or the <em>pointer to inode number</em> is \c NULL
 *  \return -\c ENOSPC, if the list of free inodes is empty
 *  \return -\c EFININVAL, if the free inode is inconsistent
 *  \return -\c EFDININVAL, if the free inode in the dirty state is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c EDCINVAL, if the data cluster header is inconsistent
 *  \return -\c EWGINODENB, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em>
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soAllocInode (uint32_t type, uint32_t* p_nInode)
{
	soColorProbe (611, "07;31", "soAllocInode (%"PRIu32", %p)\n", type, p_nInode);

	/* verificar o p_nInode é válido (aponta para um inode number) */
	if(p_nInode == NULL){
		return -EINVAL;
	}

	/* verificar se o type passado como argumento é valido, tem de ser um ficheiro, diretorio ou symbolic link */
	/* para ver as macros: docs/index.html (se não encontrares o docs vai sacar ao moodle o sofs14) depis clicar em Files > Globals > Macros e procurar por INODE e vemos que estão em sofs_indoe.h */
	/* #define INODE_TYPE_MASK (INODE_DIR | INODE_FILE | INODE_SYMLINK) */
	if((type & INODE_TYPE_MASK)==0){
		return -EINVAL;
	}

	/* temos de obter a lista de inodes livres, para isso temos de fazer load do superblock que contem a lista de inodes livres */
	SOSuperBlock *p_sb;
	int stat;

	/* lista de funções para tratamento do superbloco: sofs_basicoper.h */
	if((stat = soLoadSuperBlock())){
		return stat;
	}
	p_sb = soGetSuperBlock();

	/* depois de fazer o load do superblock temos de verificar se existem nós livres: ENOSPC, if the list of free inodes is empty */
	if(p_sb->iFree == 0){
		return -ENOSPC;
	}

	/* se há free inodes, vamos fazer load do inodes */
	/* temos o numero do inode no p_sb->iHead que tem o nó da cabeça que vamos retirar da lista de i-nos vazios */
	uint32_t nBlk, offset;
	/* obtemos o n do bloco e o offset para o inode */
	if((stat = soConvertRefInT(p_sb->iHead, &nBlk, &offset))){
		return stat;
	}
	/* vai ser guardado no p_sb->iHead então temos de atribuir o p_nInode ao iHead */
	*p_nInode = p_sb->iHead;

	SOInode *p_iNode;
	if((stat = soLoadBlockInT(nBlk))){
		return stat;
	}
	p_iNode = soGetBlockInT();

	/* EFININVAL, if the free inode is inconsistent */
	/* Quick check of a free inode. */
	/* \return <tt>0 (zero)</tt>, on success
	 * \return -\c EINVAL, if the pointer is \c NULL
	 * \return -\c EFININVAL, if the free inode is inconsistent
	 */
	if((stat = soQCheckFInode(&p_iNode[offset])) != 0){
		return stat;
	}

	/* EFDININVAL, if the free inode in the dirty state is inconsistent */
	/* primeiro temos de saber se o inode está no estado dirty */
	/* Quick check of a free inode in the clean state. */
	if((stat = soQCheckFCInode(&p_iNode[offset])) != 0){
		/* está no estado dirty mas não sabemos a sua consistência */
		/* \return -\c EINVAL, if any of the pointers is \c NULL
		 *  \return -\c EFDININVAL, if the free inode in the dirty state is inconsistent
		 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
		 *  \return -\c EDCINVAL, if the data cluster header is inconsistent
		 *  \return -\c EBADF, if the device is not already opened
		 *  \return -\c EIO, if it fails on reading or writing
		 *  \return -\c ELIBBAD, if the buffercache is inconsistent or the superblock or a data block was not previously loaded on a previous store operation
		 
		Quick check of a free inode in the dirty state.
		The contents of all the fields of the inode, except owner, group and size, are checked for consistency. Only legal values are allowed.
		*/
		if((stat = soQCheckFDInode(p_sb, &p_iNode[offset])) != 0){
			return stat;
		}

		if((stat = soCleanInode(*p_nInode)) != 0){
			return stat;
		}

		if ((stat = soLoadBlockInT (nBlk)) != 0){
      		return stat;
		}
		
		p_iNode = soGetBlockInT();

		//if ((stat = soQCheckFCInode(&p_itable[offset])) != 0) {
            //printf("nao está bem limpo fdx\n");
            //return stat;
        //}
	}

	/* if the inode is free, reference to the next inode in the double-linked list of free inodes */
	uint32_t new_head = p_iNode[offset].vD1.next;

	/* modificar as caracteristicas do inode */
	p_iNode[offset].mode = type;
	p_iNode[offset].refCount = 0;
	p_iNode[offset].owner = getuid();
	p_iNode[offset].group = getgid();
	p_iNode[offset].size = 0;
	p_iNode[offset].cluCount = 0;
	p_iNode[offset].vD1.aTime = time(NULL);
	p_iNode[offset].vD2.mTime = time(NULL);

	int i;
	for(i=0; i<N_DIRECT; i++){
		p_iNode[offset].d[i] = NULL_CLUSTER;
	}

	p_iNode[offset].i1 = NULL_CLUSTER;
	p_iNode[offset].i2 = NULL_CLUSTER;

	if((stat = soStoreBlockInT())){
		return stat;
	}

	p_sb->iFree--;
	p_sb->iHead = new_head;

	/* se não existirem mais inodes livres, iFree vai ser = 0 então temos de dizer que a iHead vai ser NULL_INODE assim como a iTail */
	if(p_sb->iFree == 0){
		p_sb->iHead = NULL_INODE;
		p_sb->iTail = NULL_INODE;
	}

	/* se a lista de inodes vazios não estiver vazia temos apenas de dizer que o próximo inode vazio tem um prev = NULL_INODE */
	if(new_head != NULL_INODE){
		if((stat = soConvertRefInT(new_head, &nBlk, &offset))){
			return stat;
		}
		if((stat = soLoadBlockInT(nBlk))){
			return stat;
		}

		p_iNode = soGetBlockInT();
		p_iNode[offset].vD2.prev = NULL_INODE;

		if((stat = soStoreBlockInT())){
			return stat;
		}
	}

	/* guardar todas as alterações no super bloco */
	if((stat = soStoreSuperBlock())){
		return stat;
	}

	/* foi tudo feito com sucesso */
	return 0;
}
