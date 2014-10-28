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

/* Allusion to external function */

int soGetDirEntryByName (uint32_t nInodeDir, const char *eName, uint32_t *p_nInodeEnt, uint32_t *p_idx);

/** \brief operation add a generic entry to a directory */
#define ADD         0
/** \brief operation attach an entry to a directory to a directory */
#define ATTACH      1

/**
 *  \brief Add a generic entry / attach an entry to a directory to a directory.
 *
 *  In the first case, a generic entry whose name is <tt>eName</tt> and whose inode number is <tt>nInodeEnt</tt> is added
 *  to the directory associated with the inode whose number is <tt>nInodeDir</tt>. Thus, both inodes must be in use and
 *  belong to a legal type, the former, and to the directory type, the latter.
 *
 *  Whenever the type of the inode associated to the entry to be added is of directory type, the directory is initialized
 *  by setting its contents to represent an empty directory.
 *
 *  In the second case, an entry to a directory whose name is <tt>eName</tt> and whose inode number is <tt>nInodeEnt</tt>
 *  is attached to the directory, the so called <em>base directory</em>, associated to the inode whose number is
 *  <tt>nInodeDir</tt>. The entry to be attached is supposed to represent itself a fully organized directory, the so
 *  called <em>subsidiary directory</em>. Thus, both inodes must be in use and belong to the directory type.
 *
 *  The <tt>eName</tt> must be a <em>base name</em> and not a <em>path</em>, that is, it can not contain the
 *  character '/'. Besides there should not already be any entry in the directory whose <em>name</em> field is
 *  <tt>eName</tt>.
 *
 *  The <em>refcount</em> field of the inode associated to the entry to be added / updated and, when required, of the
 *  inode associated to the directory are updated. This may also happen to the <em>size</em> field of either or both
 *  inodes.
 *
 *  The process that calls the operation must have write (w) and execution (x) permissions on the directory.
 *
 *  \param nInodeDir number of the inode associated to the directory
 *  \param eName pointer to the string holding the name of the entry to be added / attached
 *  \param nInodeEnt number of the inode associated to the entry to be added / attached
 *  \param op type of operation (ADD / ATTACH)
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if any of the <em>inode numbers</em> are out of range or the pointer to the string is \c NULL
 *                      or the name string does not describe a file name or no operation of the defined class is described
 *  \return -\c ENAMETOOLONG, if the name string exceeds the maximum allowed length
 *  \return -\c ENOTDIR, if the inode type whose number is <tt>nInodeDir</tt> (ADD), or both the inode types (ATTACH),
 *                       are not directories
 *  \return -\c EEXIST, if an entry with the <tt>eName</tt> already exists
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on the directory where the
 *                      entry is to be added / attached
 *  \return -\c EPERM, if the process that calls the operation has not write permission on the directory where the entry
 *                     is to be added / attached
 *  \return -\c EMLINK, if the maximum number of hardlinks in either one of inodes has already been attained
 *  \return -\c EFBIG, if the directory where the entry is to be added / attached, has already grown to its maximum size
 *  \return -\c ENOSPC, if there are no free data clusters
 *  \return -\c EDIRINVAL, if the directory is inconsistent
 *  \return -\c EDEINVAL, if the directory entry is inconsistent
 *  \return -\c EIUININVAL, if the inode in use is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c EDCMINVAL, if the mapping association of the data cluster is invalid
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soAddAttDirEntry (uint32_t nInodeDir, const char *eName, uint32_t nInodeEnt, uint32_t op)
{
  soColorProbe (313, "07;31", "soAddAttDirEntry (%"PRIu32", \"%s\", %"PRIu32", %"PRIu32")\n", nInodeDir,
                eName, nInodeEnt, op);

  	int stat;
  	SOSuperBlock *p_sosb;
  	SOInode *pInodeDir;
  	uint32_t *pIdx, outVal, clustIdx, dirEntryIdx;

  	//Load do SuperBlock
  	if((stat = soLoadSuperBlock())!=0){
    	return stat;
  	}

  	//Obter o ponteiro para o conteudo do SuperBlock
  	p_sosb = soGetSuperBlock();

  	/*****VALIDACOES*****/
  	if(((op != ATTACH) && (op != ADD)) || (nInodeDir > p_sosb->itotal) || (nInodeEnt > p_sosb->itotal) || (eName == NULL) || (strlen(eName)==0) ){
  		return -EINVAL;
  	}

  	//Validacao do nome
  	if(strlen(eName) > MAX_NAME){
  		return -ENAMETOOLONG;
  	}
  	/*
  	strcpy(tempName,eName);
  	strcpy(tempName, basename(tempName));

  	if(strcmp(eNameTmp, eName)!=0)
	return -EINVAL;
	*/

	//Leitura e validacao do inode
  	if((stat=soReadInode(pInodeDir, nInodeDir))!=0){
		return stat;		
  	}

  	if((inodeDir->mode & INODE_DIR) != INODE_DIR){
		return -ENOTDIR;
  	}

  	//Verificacao de permicoes de leitura e escrita
  	if((stat=soAccessGranted(nInodeDir,X))!=0){
		return -EACCES;
  	}

  	if((stat=soAccessGranted(nInodeDir,W))!=0){
		return -EPERM;
  	}

  	//Ver se ja atingiu tamanho maximo
  	if(pInodeDir->size >= MAX_FILE_SIZE){
		return -EFBIG;
  	}

  	/*verifica se podem ser colocadas mais referencias
 	if(inodeDir.refcount == 0xFFFF){
		return -EMLINK;  
 	}*/

	if((stat = soGetDirEntryByName(nInodeDir,eName,NULL, pIdx))==0){
		return -EEXIST;
	}
	
	clustIdx  = pIdx/DPC;
  	dirEntryIdx = pIdx%DPC;

  	/*get do data cluster com a entrada de diretorio*/
  	if((ret=soHandleFileCluster(nInodeDir,clustIdx,GET,&outVal))!=0){
		return ret;
  	}

  	 /*so aloca o cluster se nao estiver alocado*/
    if(&pOutVal==NULL_CLUSTER)
    {
        if((ret=soHandleFileCluster(nInodeDir,clustInd,ALLOC,&p_outVal))!=0)
            return ret;
        
        if((ret=soReadFileCluster(nInodeDir,clustInd,&dc))!=0)
            return ret;
        
        /* preenchimento de todas as entradas de diretorio do datacluster*/
        for(i=0; i<DPC; i++)
        {
            dc.de[i].nInode = NULL_INODE;
            memset(&(dc.de[i].name),0x00, MAX_NAME+1);
        }
        
        if((ret=soWriteFileCluster(nInodeDir,clustInd,&dc))!=0)
            return ret;
        
        /*atualizacao do tamanho do inode*/
        if((ret=soReadInode(&inodeDir, nInodeDir))!=0)
            return ret;
        
        inodeDir.size += sizeof(SODirEntry)*DPC;
        
        if((ret=soWriteInode(&inodeDir, nInodeDir))!=0)
            return ret;
    }


  return 0;
}
