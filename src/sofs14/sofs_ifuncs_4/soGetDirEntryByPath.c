/**
 *  \file soGetDirEntryByPath.c (implementation file)
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
#include "../sofs_superblock.h"
#include "../sofs_inode.h"
#include "../sofs_direntry.h"
#include "../sofs_basicoper.h"
#include "../sofs_basicconsist.h"
#include "../sofs_ifuncs_1.h"
#include "../sofs_ifuncs_2.h"
#include "../sofs_ifuncs_3.h"

/* Allusion to external function */

int soGetDirEntryByName (uint32_t nInodeDir, const char *eName, uint32_t *p_nInodeEnt, uint32_t *p_idx);

/* Allusion to internal function */

int soTraversePath (const char *ePath, uint32_t *p_nInodeDir, uint32_t *p_nInodeEnt);

/** \brief Number of symbolic links in the path */
 static uint32_t nSymLinks = 0;

/** \brief Old directory inode number */

static uint32_t oldNInodeDir = 0; 

/**
 *  \brief Get an entry by path.
 *
 *  The directory hierarchy of the file system is traversed to find an entry whose name is the rightmost component of
 *  <tt>ePath</tt>. The path is supposed to be absolute and each component of <tt>ePath</tt>, with the exception of the
 *  rightmost one, should be a directory name or symbolic link name to a path.
 *
 *  The process that calls the operation must have execution (x) permission on all the components of the path with
 *  exception of the rightmost one.
 *
 *  \param ePath pointer to the string holding the name of the path
 *  \param p_nInodeDir pointer to the location where the number of the inode associated to the directory that holds the
 *                     entry is to be stored
 *                     (nothing is stored if \c NULL)
 *  \param p_nInodeEnt pointer to the location where the number of the inode associated to the entry is to be stored
 *                     (nothing is stored if \c NULL)
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL
 *  \return -\c ENAMETOOLONG, if the path or any of the path components exceed the maximum allowed length
 *  \return -\c ERELPATH, if the path is relative and it is not a symbolic link
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT,  if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c EDIRINVAL, if the directory is inconsistent
 *  \return -\c EDEINVAL, if the directory entry is inconsistent
 *  \return -\c EIUININVAL, if the inode in use is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soGetDirEntryByPath (const char *ePath, uint32_t *p_nInodeDir, uint32_t *p_nInodeEnt)
{
  soColorProbe (311, "07;31", "soGetDirEntryByPath (\"%s\", %p, %p)\n", ePath, p_nInodeDir, p_nInodeDir);
  		int error; //Variável para verificação de erros
  		nSymLinks=0;	//Variavel para nºtotal de atalhos
  		oldNInodeDir=0; //Variavel para armazenar o nº do no onde se ficou antes do aparecimento de um atalho
  		
  		//Ponteiro para o string associado ao parametro epath não pode ser nulo
  		if(ePath==NULL)
  		{

  			return -EINVAL;
  		}

 		// O caminho tem de ser absoluto  		
		if(ePath[0]!='/')
		{
			return -ERELPATH;

		}

		//Verificar se o caminho ou qualquer dos seus componentes excede o tamanho permitido
		if(strlen(ePath)>MAX_PATH)
				{

				return ENAMETOOLONG;
				}

		// Atravessar caminho para obter DirEntry
		if((error=soTraversePath(ePath,p_nInodeDir,p_nInodeEnt))!=0)
		{
			return error;
		}


		if(p_nInodeDir!=NULL){

			*p_nInodeDir=0;
		}

		if(p_nInodeEnt!=NULL)
		{
			*p_nInodeEnt=0;
		}



  /* insert your code here */

  return 0;
}

/**
 *  \brief Traverse the path.
 *
 *  \param ePath pointer to the string holding the name of the path
 *  \param p_nInodeDir pointer to the location where the number of the inode associated to the directory that holds the
 *                     entry is to be stored
 *  \param p_nInodeEnt pointer to the location where the number of the inode associated to the entry is to be stored
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c ENAMETOOLONG, if any of the path components exceed the maximum allowed length
 *  \return -\c ERELPATH, if the path is relative and it is not a symbolic link
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT,  if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c EDIRINVAL, if the directory is inconsistent
 *  \return -\c EDEINVAL, if the directory entry is inconsistent
 *  \return -\c EIUININVAL, if the inode in use is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soTraversePath (const char *ePath, uint32_t *p_nInodeDir, uint32_t *p_nInodeEnt)
{
		
		char save_ePath[MAX_PATH+1]; //Variavel auxiliar para salvaguarda de informação
		char save2_ePath[MAX_PATH+1];//Variavel auxiliar para salvaguarda de informação
		int error;					//Variavel para verificação de erros
		SOInode inode;				//Variável do tipo SOInode
		uint32_t status;			//Verificação do estado do inode em uso
		SODataClust buff;			//Variavel do tipo SODataCluster
		char path[MAX_PATH+1];		//String auxiliar path
		char *d_name;				//dirname
		char *b_name;				//basename
		uint32_t p_idx;			  	//Indice para entrada de directório
		
		
		SOSuperBlock *p_sb;

		//Carregar superbloco para memoria interna
		if((error=soLoadSuperBlock())!=0)
		{
			return error;
		}

		//Obter ponteiro para superbloco
		p_sb=soGetSuperBlock();

		
		strcpy(save_ePath,ePath);
		
		//dirname
		d_name=dirname(save_ePath); 

		//Salvaguarda de ePath
		strcpy(save2_ePath,ePath); 

		//basename
		b_name=basename(save2_ePath);	

		//Verificar se o nome do basename não excede o maximo permitido
		if(strlen(b_name)>MAX_NAME)
		{
			return ENAMETOOLONG;
		}


		//Se dirname é diferente de '/' e '.' continua a percorrer o caminho
		if((strcmp(d_name,".")!=0) && (strcmp(d_name,"/")!=0))
		{
			// Atravessar caminho para obter DirEntry
			if((error=soTraversePath(d_name,p_nInodeDir,p_nInodeEnt))!=0)
				{
				return error;
				}


		//Atribui a *p_nInodeDir *p_nInodeEnt 		
		*p_nInodeDir=*p_nInodeEnt;
		}

		//Se dirname='.' *p_nInodeDir=0
		if(strcmp(d_name,".")==0)
		{
			*p_nInodeDir=oldNInodeDir;
		}

		//Se dirname='/' *p_nInodeDir=0
		if(strcmp(d_name,"/")==0)
		{
			*p_nInodeDir=0;	
		}
		//Se basename='/', tem de ser alterado para '.'
		if(strcmp(b_name,"/")==0)
		{
			strcpy(b_name,".");	
		}

		//Nos-i dos diversos componentes de encaminhamento tem que estar em uso
		if((status=soQCheckInodeIU(p_sb,&inode))!=0){
			return -ELDCININVAL;
		}

		//Ler um no-i da tabela de inodes que tem de corresponder a tipos de ficheiros válidos
		if((error=soReadInode(&inode,*p_nInodeDir,status))!=0)
		{
			return error;
		}

		//Verificar consistência do directório
		if((soQCheckDirCont(p_sb,&inode))!=0){
			return -EDIRINVAL;
		}

		//Ver se o no-i corresponde a um directorio
		if((inode.mode & INODE_DIR)!=INODE_DIR)
		{
			return -ENOTDIR;
		}

		//Verificar permissões de execução
		if((soAccessGranted(*p_nInodeDir,X))!=0)
		{
				return -EACCES;
		}

		//Obter nome da entrada de directorio
		if((soGetDirEntryByName(*p_nInodeDir,b_name,p_nInodeEnt,&p_idx))!=0)
		{
			return -ENOENT;
		}



		//Nos-i dos diversos componentes de encaminhamento tem que estar em uso
		if((status=soQCheckInodeIU(p_sb,&inode))!=0){
			return status;
		}

		
		//Ler um no-i da tabela de inodes que tem de corresponder a tipos de ficheiros vàlidos
		if((error=soReadInode(&inode,*p_nInodeEnt,status))!=0)
		{
			return error;
		}

		//Verificar consistência da entrada de directório
		if((soQCheckDirCont(p_sb,&inode))!=0){
			return -EDEINVAL;
		}

		//Ver se o no-i corresponde a um atalho
		if((inode.mode & INODE_SYMLINK)==INODE_SYMLINK)
			{
				//Se for só um atalho para aqui o programa
				if(nSymLinks==1){
				return -ELOOP;
			}

				if(nSymLinks == 0)
			nSymLinks = 1;

		//Se houver mais que um atalho, ver permissões de execução e leitura
		if((error=soAccessGranted(*p_nInodeEnt,R+X))!=0)
		{
				return error;
		}

		//Ler cluster correspondente à entrada de directório	
		if((error=soReadFileCluster(*p_nInodeEnt,p_idx/DPC,&buff))!=0)
		{
			return error;
		}
	
		//strncpy(path,buff.info.data,MAX_PATH+1);
		memcpy(path,buff.info.data,MAX_PATH+1);

		//Guardar em oldNInodeDir o valor de *p_nInodeDir(onde ficou antes do atalho)
		oldNInodeDir=*p_nInodeDir;

		if((error=soTraversePath(path,p_nInodeDir,p_nInodeEnt))!=0){

					return error;
}

		}
		//Enviar informação do superbloco para o dispositivo
		if((soStoreSuperBlock())!=0)
		{
			return -ELIBBAD;
		}





/* insert your code here */

  return 0;
}
