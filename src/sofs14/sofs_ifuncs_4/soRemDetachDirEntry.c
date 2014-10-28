/**
 *  \file soAddAttDirEntry.c (implementation file)
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
int soCheckDirectoryEmptiness (uint32_t nInodeDir);

/** \brief operation remove a generic entry from a directory */
#define REM         0
/** \brief operation detach a generic entry from a directory */
#define DETACH      1

/**
 *  \brief Remove / detach a generic entry from a directory.
 *
 *  The entry whose name is <tt>eName</tt> is removed / detached from the directory associated with the inode whose
 *  number is <tt>nInodeDir</tt>. Thus, the inode must be in use and belong to the directory type.
 *
 *  Removal of a directory entry means exchanging the first and the last characters of the field <em>name</em>.
 *  Detachment of a directory entry means filling all the characters of the field <em>name</em> with the \c NULL
 *  character and making the field <em>nInode</em> equal to \c NULL_INODE.
 *
 *  The <tt>eName</tt> must be a <em>base name</em> and not a <em>path</em>, that is, it can not contain the
 *  character '/'. Besides there should exist an entry in the directory whose <em>name</em> field is <tt>eName</tt>.
 *
 *  Whenever the operation is removal and the type of the inode associated to the entry to be removed is of directory
 *  type, the operation can only be carried out if the directory is empty.
 *
 *  The <em>refcount</em> field of the inode associated to the entry to be removed / detached and, when required, of
 *  the inode associated to the directory are updated.
 *
 *  The file described by the inode associated to the entry to be removed / detached is only deleted from the file
 *  system if the <em>refcount</em> field becomes zero (there are no more hard links associated to it) and the operation
 *  is removal. In this case, the data clusters that store the file contents and the inode itself must be freed.
 *
 *  The process that calls the operation must have write (w) and execution (x) permissions on the directory.
 *
 *  \param nInodeDir number of the inode associated to the directory
 *  \param eName pointer to the string holding the name of the directory entry to be removed / detached
 *  \param op type of operation (REM / DETACH)
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>inode number</em> is out of range or the pointer to the string is \c NULL or the
 *                      name string does not describe a file name or no operation of the defined class is described
 *  \return -\c ENAMETOOLONG, if the name string exceeds the maximum allowed length
 *  \return -\c ENOTDIR, if the inode type whose number is <tt>nInodeDir</tt> is not a directory
 *  \return -\c ENOENT,  if no entry with <tt>eName</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on the directory
 *  \return -\c EPERM, if the process that calls the operation has not write permission on the directory
 *  \return -\c ENOTEMPTY, if the entry with <tt>eName</tt> describes a non-empty directory
 *  \return -\c EDIRINVAL, if the directory is inconsistent
 *  \return -\c EDEINVAL, if the directory entry is inconsistent
 *  \return -\c EIUININVAL, if the inode in use is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soRemDetachDirEntry (uint32_t nInodeDir, const char *eName, uint32_t op)
{
  soColorProbe (314, "07;31", "soRemDetachDirEntry (%"PRIu32", \"%s\", %"PRIu32")\n", nInodeDir, eName, op);

  if(op != REM && op != DETACH){
	return -EINVAL;
  }

  uint32_t idxDir, nInodeEnt;
  int stat, i;
  SOInode inodeDir, inodeEntry;
  SODataClust dc;

  /*
  Vou fazer load do inode, a função soReadInode já verifica se o número do inode
  é valido ou não. O inode tem de estar em uso.
  */
  if((stat = soReadInode(&inodeDir, nInodeDir, IUIN))){
	return stat;
  }

  if((inodeDir.mode & INODE_DIR) != INODE_DIR){
	return -ENOTDIR;
  }

  /* 
	Precisamos de permissões de escrita e execução
  */
  if((stat = soAccessGranted(nInodeDir, X))){
	return stat;
  }
  if((stat = soAccessGranted(nInodeDir, W))){
	return stat;
  }

  /*
  O eName tem de ser um base name e não um path. Tem de existir um dir com o name = eName
  Vamos obter o nº do InodeEntry que procuramos com o eName. 
  */
  if(eName == NULL){
    return -EINVAL;
  } 
  if(strlen(eName) == 0){
    return -EINVAL;
  }
  if(strlen(eName) > MAX_NAME){
    return -ENAMETOOLONG;
  }
  if(strchr(eName, '/') != 0){
  	return -EINVAL;
  }
    
  /*
	o soGetDirEntryByName tem de ter permissão de execução no diretório para o atravessar, já faz check
  */
  if((stat = soGetDirEntryByName(nInodeDir, eName, &nInodeEnt, &idxDir))){
	return stat;
  }

  /* 
	Sempre que a operação for remover e o tipo do inode associado à dir que vai ser removida for do tipo directory, a operação apenas pode ser executada se o diretório estiver vazio.
  */
  if((stat = soReadInode(&inodeEntry, nInodeEnt, IUIN))){
	return stat;
  }
  
  if(op == REM){
	if((inodeEntry.mode & INODE_DIR)){
	  if((stat = soCheckDirectoryEmptiness(nInodeEnt))){
		return stat;
	  }
	  //remove reference a ela propria
	  inodeEntry.refCount--;
	  //remove reference ..
	  inodeDir.refCount--;
	}

	if((stat = soReadFileCluster(nInodeDir, (idxDir/DPC), &dc))){
	  return stat;
	}

	dc.info.de[idxDir%DPC].name[MAX_NAME] = dc.info.de[idxDir%DPC].name[0];
	dc.info.de[idxDir%DPC].name[0]='\0';

	inodeEntry.refCount--;
	
	if((stat = soWriteFileCluster(nInodeDir, idxDir/DPC, &dc))){
	  return stat;
	}

	//se refcount == 0 temos de fazer free aos data clusters e aos inodes
	if(inodeEntry.refCount == 0){  
	  if((stat = soWriteInode(&inodeEntry, nInodeEnt, IUIN))){
		return stat;
	  }

	  if((stat = soHandleFileClusters(nInodeEnt, 0, FREE))){
		return stat;
	  }
	  
	  if((stat = soFreeInode(nInodeEnt))){
		return stat;
	  }
		
	  if((stat = soWriteInode(&inodeDir, nInodeDir, IUIN))){
	    return stat;
	  }		
	}else{ 
	  if((stat = soWriteInode(&inodeEntry, nInodeEnt, IUIN))){
		return stat;
	  }
	  
	  if((stat = soWriteInode(&inodeDir, nInodeDir, IUIN))){
	    return stat;
	  }
	}

	

  }else{
	/* DETACH */
	if((inodeEntry.mode & INODE_DIR) == INODE_DIR){
	  inodeEntry.refCount--;
	  inodeDir.refCount--;
	}

	if((stat=soReadFileCluster(nInodeDir, (idxDir/DPC), &dc))){
	  return stat; 
	}
			   
	inodeEntry.refCount--;

	for(i = 0; i < MAX_NAME; i++){
	  dc.info.de[idxDir % DPC].name[i] = '\0';
	}
	
	dc.info.de[idxDir % DPC].nInode = NULL_INODE;

	if((stat = soWriteFileCluster(nInodeDir, (idxDir / DPC), &dc))){
	  return stat;
	}
	
	if((stat = soWriteInode(&inodeEntry, nInodeEnt, IUIN))){
	  return stat;
	}
  
	if((stat = soWriteInode(&inodeDir, nInodeDir, IUIN))){
	  return stat;
	}
			
  }

  return 0;
}
