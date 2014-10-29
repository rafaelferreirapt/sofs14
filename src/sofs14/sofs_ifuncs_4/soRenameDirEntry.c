/**
 *  \file soRenameDirEntry.c (implementation file)
 *
 *  \author
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>
#include <libgen.h>
#include <string.h>

#include "sofs_probe.h"
#include "sofs_buffercache.h"
#include "sofs_superblock.h"
#include "sofs_inode.h"
#include "sofs_direntry.h"
#include "sofs_basicoper.h"
#include "sofs_basicconsist.h"
#include "sofs_ifuncs_1.h"
#include "sofs_ifuncs_2.h"
#include "sofs_ifuncs_3.h"

/* Allusion to external functions */

int soGetDirEntryByName (uint32_t nInodeDir, const char *eName, uint32_t *p_nInodeEnt, uint32_t *p_idx);

/**
 *  \brief Rename an entry of a directory.
 *
 *  The directory entry whose name is <tt>oldName</tt> has its <em>name</em> field changed to <tt>newName</tt>. Thus,
 *  the inode associated to the directory must be in use and belong to the directory type.
 *
 *  Both the <tt>oldName</tt> and the <tt>newName</tt> must be <em>base names</em> and not <em>paths</em>, that is,
 *  they can not contain the character '/'. Besides an entry whose <em>name</em> field is <tt>oldName</tt> should exist
 *  in the directory and there should not be any entry in the directory whose <em>name</em> field is <tt>newName</tt>.
 *
 *  The process that calls the operation must have write (w) and execution (x) permissions on the directory.
 *
 *  \param nInodeDir number of the inode associated to the directory
 *  \param oldName pointer to the string holding the name of the direntry to be renamed
 *  \param newName pointer to the string holding the new name
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>inode number</em> is out of range or either one of the pointers to the strings are
 *                      \c NULL or the name strings do not describe file names
 *  \return -\c ENAMETOOLONG, if one of the name strings exceeds the maximum allowed length
 *  \return -\c ENOTDIR, if the inode type is not a directory
 *  \return -\c ENOENT,  if no entry with <tt>oldName</tt> is found
 *  \return -\c EEXIST,  if an entry with the <tt>newName</tt> already exists
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on the directory
 *  \return -\c EPERM, if the process that calls the operation has not write permission on the directory
 *  \return -\c EDIRINVAL, if the directory is inconsistent
 *  \return -\c EDEINVAL, if the directory entry is inconsistent
 *  \return -\c EIUININVAL, if the inode in use is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soRenameDirEntry (uint32_t nInodeDir, const char *oldName, const char *newName)
{
  soColorProbe (315, "07;31", "soRenameDirEntry (%"PRIu32", \"%s\", \"%s\")\n", nInodeDir, oldName, newName);
  SOSuperBlock *p_sb;
  SOInode inode;
  SODataClust dirClust;
  int size;
  int stat;
  uint32_t p_idx;
  //uint32_t p_nInodeEnt;

  // obter informação do superbloco
  if((stat=soLoadSuperBlock())!=0)
  {
  	return stat;
  }
  // ponteiro para o superblock
  p_sb=soGetSuperBlock();

  // verificação de validade do iNode
  if(nInodeDir>=p_sb->iTotal || nInodeDir < 0)
  {
  	return -EINVAL;
  }

  if (oldName == NULL || newName == NULL)
  {
  	return -EINVAL;
  }

  // check ao tamanho do nome antigo
  size = strlen(oldName);
  if(size > (MAX_NAME))
  {
  	return -ENAMETOOLONG;
  }
  // check ao tamanho do novo nome
  size = strlen(newName);
  if(size > (MAX_NAME))
  {
  	return -ENAMETOOLONG;
  }
  /* verificaçã odas string, estas nao podem
  conter '/' no seu conteudo 
  char test;
  test=strchr(&newName,'/');
  if(test != NULL)
  {
  	return -EINVAL;
  }
  test=strchr(&oldName,'/');
  {
  	return -EINVAL
  }
  esta verificação e feita pela funçao pela soGetEntryByName
	*/	

  // leitura do inode
  if ((stat=soReadInode(&inode,nInodeDir,IUIN))!=0)
  {
  	return stat;
  }

  if((stat=soQCheckInodeIU(p_sb,&inode))!=0)
  {
  	return stat;
  }
  // verificação de permissoes de execução
  if((stat=soAccessGranted(nInodeDir,X))!=0)
  {
  	return -EACCES;
  }
  // verificação de permissoes de escrita
  if((stat=soAccessGranted(nInodeDir,W))!=0)
  {
  	return -EPERM;
  }

  // verificação se o inodeDir e um directorio
  if((inode.mode & INODE_DIR)!= INODE_DIR)
  {
  	return -ENOTDIR;
  }
  /*if((stat=soQCheckDirCont(p_sb,&inode))!=0)
  {
    return stat;
  }
  perror("antesQcheck");*/
  //verificação se o novo nome a alterar existe
  if((stat=soGetDirEntryByName(nInodeDir,oldName,NULL,&p_idx))!= 0){
    return stat;
  }
  

  if((stat=soGetDirEntryByName(nInodeDir,newName,NULL,NULL)) == 0){
    return -EEXIST;
  }
  /*if(stat != 0)
  {
    return stat;
  }*/

  /* vamos carregar o cluster que possui a entrada de directorio que pretendemos renomear*/
  if((stat=soReadFileCluster(nInodeDir,p_idx/DPC, &dirClust))!=0)
  {
  	return stat;
  }
  // copiar nova string para a entrada onde se encontrava o oldName
  //memcpy((char*)dirClust.info.de[p_idx%DPC].name,(char*)newName,MAX_NAME+1);
  int i = 0;
  for(i=0;i < MAX_NAME+1;i++)
  {
    dirClust.info.de[p_idx%DPC].name[i]=0;
  }
  strcpy((char*)dirClust.info.de[p_idx%DPC].name,(char*)newName);
  if((stat=soWriteFileCluster(nInodeDir,p_idx/DPC, &dirClust))!=0)
  {
  	return stat;
  }

  return 0;

}

