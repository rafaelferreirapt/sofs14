/**
 *  \file soAccessGranted.c (implementation file)
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
#include "sofs_basicoper.h"
#include "sofs_basicconsist.h"

/** \brief inode in use status */
#define IUIN  0
/** \brief free inode in dirty state status */
#define FDIN  1

/** \brief performing a read operation */
#define R  0x0004
/** \brief performing a write operation */
#define W  0x0002
/** \brief performing an execute operation */
#define X  0x0001

/* allusion to internal function */

int soReadInode (SOInode *p_inode, uint32_t nInode, uint32_t status);

/**
 *  \brief Check the inode access rights against a given operation.
 *
 *  The inode must to be in use and belong to one of the legal file types.
 *  It checks if the inode mask permissions allow a given operation to be performed.
 *
 *  When the calling process is <em>root</em>, access to reading and/or writing is always allowed and access to
 *  execution is allowed provided that either <em>user</em>, <em>group</em> or <em>other</em> have got execution
 *  permission.
 *
 *  \param nInode number of the inode
 *  \param opRequested operation to be performed:
 *                    a bitwise combination of R, W, and X
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if <em>buffer pointer</em> is \c NULL or no operation of the defined class is described
 *  \return -\c EACCES, if the operation is denied
 *  \return -\c EIUININVAL, if the inode in use is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c EDCINVAL, if the data cluster header is inconsistent
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soAccessGranted (uint32_t nInode, uint32_t opRequested)
{
  soColorProbe (514, "07;31", "soAccessGranted (%"PRIu32", %"PRIu32")\n", nInode, opRequested);

  uint32_t block;
  uint32_t offset;
  uint16_t validType;
  SOSuperBlock *p_sosb;
  int stat;

  /*Load superblock*/
  if((stat=soLoadSuperBlock()) != 0 ){
     return stat;
  }

  p_sosb=soGetSuperBlock();

  /*Validaçao de conformidade*/
  if(nInode >= p_sosb->iTotal || nInode < 0){    /*o número do nó-i tem que ser um valor válido*/
     return -EINVAL;
  }

  if(opRequested > (R | W | X) || opRequested <= 0){  /*a operação solicitada, ou a combinação de operações solicitadas, têm que pertencer à classe predefinida.*/
    return -EINVAL;
  }

  /*Validação de consistencia*/

  if((stat = soConvertRefInT(nInode, &block, &offset)) != 0){
    return stat;
  }
  if((stat = soLoadBlockInT(block)) != 0){
    return stat;
  }

  SOInode *p_ind = soGetBlockInT();

  if ((soQCheckInodeIU(p_sosb, &p_ind[offset]))) return -ELIBBAD;

  if(p_ind[offset].mode & INODE_FREE) return -EIUININVAL; /*verifica a consistÊncia, se um iNode está a free, depois de se dizer que está em uso*/


  uint32_t owner = p_ind[offset].owner;      /*ID do usuário do dono do ficheiro*/
  uint32_t group = p_ind[offset].group;      /*ID do grupo do dono do ficheiro */


  validType = p_ind[offset].mode & INODE_TYPE_MASK; /*Verificar se o no-i esta associado a um tipo valido(ficheiro, diretorio, atalho)*/

  if((validType != INODE_DIR && validType != INODE_FILE && validType != INODE_SYMLINK) || p_ind == NULL){
      return -EINVAL;
  }

  if(getuid()==0)				//Para o root, R e W são sempre permitidos
  {
    if(opRequested & X)		//Mas X só é permitido, se em group, own ou other for permitido
    {
      if((((p_ind[offset].mode & INODE_EX_USR) == INODE_EX_USR) | ((p_ind[offset].mode & INODE_EX_GRP) == INODE_EX_GRP) | ((p_ind[offset].mode & INODE_EX_OTH) == INODE_EX_OTH)) != true){
          return -EACCES;
      }
    }
  }
  else if(getuid()!=0){   //Para quando não é o root
    if(getuid()==owner){
      if (opRequested & R){			// R - Permissão de Leitura
        if ((p_ind[offset].mode & INODE_RD_USR)!=INODE_RD_USR){
          return -EACCES;
        }
      }
      if (opRequested & W){			// W - Permissão de Escrita
        if ((p_ind[offset].mode & INODE_WR_USR)!=INODE_WR_USR){
          return -EACCES;
        }
      }
      if (opRequested & X){		// X - Permissão de Execução
        if ((p_ind[offset].mode & INODE_EX_USR)!=INODE_EX_USR){
          return -EACCES;
        }
      }
    }

    else if(getgid()==group){
      if (opRequested & R){			// R - Permissão de Leitura
        if ((p_ind[offset].mode & INODE_RD_GRP)!=INODE_RD_GRP){
          return -EACCES;
        }
      }
      if (opRequested & W){			// W - Permissão de Escrita
        if ((p_ind[offset].mode & INODE_WR_GRP)!=INODE_WR_GRP){
          return -EACCES;
        }
      }
      if (opRequested & X){		// X - Permissão de Execução
        if ((p_ind[offset].mode & INODE_EX_GRP)!=INODE_EX_GRP){
          return -EACCES;
        }
      }
    }

    else if(getuid()!=owner && getgid()!=group){
      if (opRequested & R){			// R - Permissão de Leitura
        if ((p_ind[offset].mode & INODE_RD_OTH)!=INODE_RD_OTH){
          return -EACCES;
        }
      }
      if (opRequested & W){			// W - Permissão de Escrita
        if ((p_ind[offset].mode & INODE_WR_OTH)!=INODE_WR_OTH){
          return -EACCES;
        }
      }
      if (opRequested & X){		// X - Permissão de Execução
        if ((p_ind[offset].mode & INODE_EX_OTH)!=INODE_EX_OTH){
          return -EACCES;
        }
      }
    }
  }

  /*Store bloco de inodes*/
  if((stat = soStoreBlockInT()) != 0){
    return stat;
  }
  return 0;
}
