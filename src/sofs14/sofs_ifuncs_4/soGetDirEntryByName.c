/**
 *  \file soGetDirEntryByName.c (implementation file)
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

/**
 *  \brief Get an entry by name.
 *
 *  The directory contents, seen as an array of directory entries, is parsed to find an entry whose name is
 *  <tt>eName</tt>. Thus, the inode associated to the directory must be in use and belong to the directory type.
 *
 *  The <tt>eName</tt> must also be a <em>base name</em> and not a <em>path</em>, that is, it can not contain the
 *  character '/'.
 *
 *  The process that calls the operation must have execution (x) permission on the directory.
 *
 *  \param nInodeDir number of the inode associated to the directory
 *  \param eName pointer to the string holding the name of the directory entry to be located
 *  \param p_nInodeEnt pointer to the location where the number of the inode associated to the directory entry whose
 *                     name is passed, is to be stored
 *                     (nothing is stored if \c NULL)
 *  \param p_idx pointer to the location where the index to the directory entry whose name is passed, or the index of
 *               the first entry that is free in the clean state, is to be stored
 *               (nothing is stored if \c NULL)
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>inode number</em> is out of range or the pointer to the string is \c NULL or the
 *                      name string does not describe a file name
 *  \return -\c ENAMETOOLONG, if the name string exceeds the maximum allowed length
 *  \return -\c ENOTDIR, if the inode type is not a directory
 *  \return -\c ENOENT,  if no entry with <tt>name</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on the directory
 *  \return -\c EDIRINVAL, if the directory is inconsistent
 *  \return -\c EDEINVAL, if the directory entry is inconsistent
 *  \return -\c EIUININVAL, if the inode in use is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soGetDirEntryByName (uint32_t nInodeDir, const char *eName, uint32_t *p_nInodeEnt, uint32_t *p_idx)
{
  soColorProbe (312, "07;31", "soGetDirEntryByName (%"PRIu32", \"%s\", %p, %p)\n",
                nInodeDir, eName, p_nInodeEnt, p_idx);

	SOSuperBlock *p_sb;
	SOInode p_Inode;
	SODataClust clustDir;
	// Variavel usada para retorno de erros
	uint32_t status;
	int i, j;

	if((status = soLoadSuperBlock())!= 0){
		return status;
	}
	
	p_sb = soGetSuperBlock();

	
	if(nInodeDir >= p_sb->iTotal){
		return -EINVAL;
	}

	// Verificação se o campo eName é igual a NULL ou esta vazio
	if(eName == NULL || (strlen(eName))==0){
		return -EINVAL;
	}

	// Verifica se a string nome excede o comprimento máximo
	if(strlen(eName) > MAX_NAME){
		return -ENAMETOOLONG;
	}

  	// Verifica se a string é um nome de um ficheiro e não um caminho
	for(i=0; eName[i]!='\0';i++) {
		if(eName[i] == '/')
			return -EINVAL;
   }

	// Le o inode. Verifica se está em uso
	if((status = soReadInode(&p_Inode,nInodeDir,IUIN))){
		return status;
	}

	// Verifica se o nó-i é do tipo diretório
	if((p_Inode.mode & INODE_TYPE_MASK) != INODE_DIR){
		return -ENOTDIR;
	}

	if((status = soQCheckDirCont(p_sb,&p_Inode))!=0){
		return status;
	}

	// Verifica se existe permissão de execução
	if((status = soAccessGranted(nInodeDir,X))!=0){
		return status;
	}


	// Numero de entradas de directorio
	uint32_t numRefMax = p_Inode.size/(DPC*sizeof(SODirEntry));

	for (i = 0; i < numRefMax; i++){
		//Le o conteudo do cluster referenciado pelo inode
		if((status = soReadFileCluster(nInodeDir,i,&clustDir))){
			return status;
		}

    // Procura pelo nome, verificando todas as entradas de directorio
		for (j = 0; j < DPC; j++){
			if (strcmp((char*)(clustDir.info.de[j].name),eName)==0){
				// Se o argumento é diferente de NULL retorna o inode referenciado pelo nome
				if(p_nInodeEnt != NULL){ 
					*p_nInodeEnt = (clustDir.info.de[j].nInode);
				}
					if(p_idx != NULL){ 
						*p_idx =((i*DPC)+j);
					}
				return 0;
			}

			// Verifica se a entrada j esta free
			if((clustDir.info.de[j].name[0] == '\0') && (clustDir.info.de[j].name[MAX_NAME] == '\0')){
				if(p_idx != NULL){
					*p_idx = ((i*DPC)+j);
				}
				return -ENOENT;
			}
		}
	}
  
  // Se nao existir entradas free
	if(p_idx != NULL){
		*p_idx = (p_Inode.cluCount)*DPC;
	}
	
  return -ENOENT;
}
