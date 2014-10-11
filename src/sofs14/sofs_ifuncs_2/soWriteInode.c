/**
 *  \file soWriteInode.c (implementation file)
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

/** \brief inode in use status */
#define IUIN  0
/** \brief free inode in dirty state status */
#define FDIN  1

/**
 *  \brief Write specific inode data to the table of inodes.
 *
 *  The inode must be in use and belong to one of the legal file types.
 *  Upon writing, the <em>time of last file modification</em> and <em>time of last file access</em> fields are set to
 *  current time, if the inode is in use.
 *
 *  \param p_inode pointer to the buffer containing the data to be written from
 *  \param nInode number of the inode to be written into
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

int soWriteInode (SOInode *p_inode, uint32_t nInode, uint32_t status)
{
	soColorProbe (512, "07;31", "soWriteInode (%p, %"PRIu32", %"PRIu32")\n", p_inode, nInode, status);

	int status_error;		// Variavel usada para o returno de erros
	SOSuperBlock *p_sb;		// Ponteiro para o SuperBlock

	// Tenta fazer o load do SuperBlock
	if((status_error = soLoadSuperBlock()) != 0){
		return status_error;
	}
	// Adquire o ponteiro para o SuperBlock
	p_sb=soGetSuperBlock(); 

	// Verifica a consistência do SuperBlock
	if((status_error = soQCheckSuperBlock(p_sb)) != 0){
		return status_error;
	}

	// Validação de conformidade:
	// o número do nó-i tem que ser um valor válido;
	// o ponteiro para a região de armazenamento do nó-i a ser lido não pode ser nulo.
	// o status de escrita tem que ser válido (nó-i em uso ou nó-i livre no estado sujo); 
	if(nInode < 0 || nInode > p_sb->iTotal || p_inode == NULL || (status != IUIN && status != FDIN)){
		return -EINVAL;
	}


	uint32_t nBlk;             // Usado para obter o numero de blocos
	uint32_t offset;           // Usado para obter o offset o inode
	SOInode *p_to_inode;      // Ponteiro para o inode onde vamos escrever

	if((status_error = soConvertRefInT(nInode, &nBlk, &offset)) != 0 ){ 
		return status_error;
	}

	if((status_error = soLoadBlockInT(nBlk)) != 0 ){
		return status_error;
	}

	p_to_inode = soGetBlockInT();

	if(status == IUIN){      // Se o nó-i está em uso...
		// Verifica a consistência do inode usado
		if ((status_error = soQCheckInodeIU(p_sb, p_inode)) != 0){
			return status_error;
		}
		p_inode->vD1.aTime = time(NULL);       
		p_inode->vD2.mTime = time(NULL);
		// Actualização do tempo do último acesso e do tempo da última modificação (apenas quando se trata de um nó-i em uso!!!)

	}else if(status == FDIN){   // Se o nó-i está no estado sujo...
		// Verifica a consistência dos "inodes sujos"
		if ((status_error = soQCheckFDInode(p_sb, p_inode)) != 0){
			return status_error;
		}
	}

	memmove(&p_to_inode[offset], p_inode, sizeof(SOInode));

	// Store da informação
	if ((status_error = soStoreBlockInT()) != 0){
		return status_error;
	}

	return 0;
}
