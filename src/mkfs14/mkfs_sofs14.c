/**
 *  \file mkfs_sofs14.c (implementation file)
 *
 *  \brief The SOFS14 formatting tool.
 *
 *  It stores in predefined blocks of the storage device the file system metadata. With it, the storage device may be
 *  envisaged operationally as an implementation of SOFS14.
 *
 *  The following data structures are created and initialized:
 *     \li the superblock
 *     \li the table of inodes
 *     \li the data zone
 *     \li the contents of the root directory seen as empty.
 *
 *  SINOPSIS:
 *  <P><PRE>                mkfs_sofs14 [OPTIONS] supp-file
 *
 *                OPTIONS:
 *                 -n name --- set volume name (default: "SOFS14")
 *                 -i num  --- set number of inodes (default: N/8, where N = number of blocks)
 *                 -z      --- set zero mode (default: not zero)
 *                 -q      --- set quiet mode (default: not quiet)
 *                 -h      --- print this help.</PRE>
 *
 *  \author Artur Carneiro Pereira - September 2008
 *  \author Miguel Oliveira e Silva - September 2009
 *  \author António Rui Borges - September 2010 - August 2011, September 2014
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "sofs_const.h"
#include "sofs_buffercache.h"
#include "sofs_superblock.h"
#include "sofs_inode.h"
#include "sofs_direntry.h"
#include "sofs_basicoper.h"
#include "sofs_basicconsist.h"

/* Allusion to internal functions */

static int fillInSuperBlock (SOSuperBlock *p_sb, uint32_t ntotal, uint32_t itotal, uint32_t nclusttotal,
		unsigned char *name);
static int fillInINT (SOSuperBlock *p_sb);
static int fillInRootDir (SOSuperBlock *p_sb);
static int fillInGenRep (SOSuperBlock *p_sb, int zero);
static int checkFSConsist (void);
static void printUsage (char *cmd_name);
static void printError (int errcode, char *cmd_name);

/* The main function */

int main (int argc, char *argv[])
{
	char *name = "SOFS14";                         /* volume name */
	uint32_t itotal = 0;                           /* total number of inodes, if kept set value automatically */
	int quiet = 0;                                 /* quiet mode, if kept set not quiet mode */
	int zero = 0;                                  /* zero mode, if kept set not zero mode */

	/* process command line options */

	int opt;                                       /* selected option */

	do
	{ switch ((opt = getopt (argc, argv, "n:i:qzh")))
		{ case 'n': /* volume name */
			name = optarg;
			break;
			case 'i': /* total number of inodes */
			if (atoi (optarg) < 0)
			{ fprintf (stderr, "%s: Negative inodes number.\n", basename (argv[0]));
				printUsage (basename (argv[0]));
				return EXIT_FAILURE;
			}
			itotal = (uint32_t) atoi (optarg);
			break;
			case 'q': /* quiet mode */
			quiet = 1;                       /* set quiet mode for processing: no messages are issued */
			break;
			case 'z': /* zero mode */
			zero = 1;                        /* set zero mode for processing: the information content of all free
							    data clusters are set to zero */
			break;
			case 'h': /* help mode */
			printUsage (basename (argv[0]));
			return EXIT_SUCCESS;
			case -1:  break;
			default:  fprintf (stderr, "%s: Wrong option.\n", basename (argv[0]));
				  printUsage (basename (argv[0]));
				  return EXIT_FAILURE;
		}
	} while (opt != -1);
	if ((argc - optind) != 1)                      /* check existence of mandatory argument: storage device name */
	{ fprintf (stderr, "%s: Wrong number of mandatory arguments.\n", basename (argv[0]));
		printUsage (basename (argv[0]));
		return EXIT_FAILURE;
	}

	/* check for storage device conformity */

	char *devname;                                 /* path to the storage device in the Linux file system */
	struct stat st;                                /* file attributes */

	devname = argv[optind];
	if (stat (devname, &st) == -1)                 /* get file attributes */
	{ printError (-errno, basename (argv[0]));
		return EXIT_FAILURE;
	}
	if (st.st_size % BLOCK_SIZE != 0)              /* check file size: the storage device must have a size in bytes
							  multiple of block size */
	{ fprintf (stderr, "%s: Bad size of support file.\n", basename (argv[0]));
		return EXIT_FAILURE;
	}

	/* evaluating the file system architecture parameters
	 * full occupation of the storage device when seen as an array of blocks supposes the equation bellow
	 *
	 *    NTBlk = 1 + NBlkTIN + NTClt*BLOCKS_PER_CLUSTER
	 *
	 *    where NTBlk means total number of blocks
	 *          NTClt means total number of clusters of the data zone
	 *          NBlkTIN means total number of blocks required to store the inode table
	 *          BLOCKS_PER_CLUSTER means number of blocks which fit in a cluster
	 *
	 * has integer solutions
	 * this is not always true, so a final adjustment may be made to the parameter NBlkTIN to warrant this
	 */

	uint32_t ntotal;                               /* total number of blocks */
	uint32_t iblktotal;                            /* number of blocks of the inode table */
	uint32_t nclusttotal;                          /* total number of clusters */

	ntotal = st.st_size / BLOCK_SIZE;
	if (itotal == 0) itotal = ntotal >> 3;
	if ((itotal % IPB) == 0)
		iblktotal = itotal / IPB;
	else iblktotal = itotal / IPB + 1;
	nclusttotal = (ntotal - 1 - iblktotal) / BLOCKS_PER_CLUSTER;
	/* final adjustment */
	iblktotal = ntotal - 1 - nclusttotal * BLOCKS_PER_CLUSTER;
	itotal = iblktotal * IPB;

	/* formatting of the storage device is going to start */

	SOSuperBlock *p_sb;                            /* pointer to the superblock */
	int status;                                    /* status of operation */

	if (!quiet)
		printf("\e[34mInstalling a %"PRIu32"-inodes SOFS11 file system in %s.\e[0m\n", itotal, argv[optind]);

	/* open a buffered communication channel with the storage device */

	if ((status = soOpenBufferCache (argv[optind], BUF)) != 0)
	{ printError (status, basename (argv[0]));
		return EXIT_FAILURE;
	}

	/* read the contents of the superblock to the internal storage area
	 * this operation only serves at present time to get a pointer to the superblock storage area in main memory
	 */

	if ((status = soLoadSuperBlock ()) != 0)
		return status;
	p_sb = soGetSuperBlock ();

	/* filling in the superblock fields:
	 *   magic number should be set presently to 0xFFFF, this enables that if something goes wrong during formating, the
	 *   device can never be mounted later on
	 */

	if (!quiet)
	{ printf ("Filling in the superblock fields ... ");
		fflush (stdout);                          /* make sure the message is printed now */
	}

	if ((status = fillInSuperBlock (p_sb, ntotal, itotal, nclusttotal, (unsigned char *) name)) != 0)
	{ printError (status, basename (argv[0]));
		soCloseBufferCache ();
		return EXIT_FAILURE;
	}

	if (!quiet) printf ("done.\n");

	/* filling in the inode table:
	 *   only inode 0 is in use (it describes the root directory)
	 */

	if (!quiet)
	{ printf ("Filling in the inode table ... ");
		fflush (stdout);                          /* make sure the message is printed now */
	}

	if ((status = fillInINT (p_sb)) != 0)
	{ printError (status, basename (argv[0]));
		soCloseBufferCache ();
		return EXIT_FAILURE;
	}

	if (!quiet) printf ("done.\n");

	/* filling in the contents of the root directory:
	 *   the first 2 entries are filled in with "." and ".." references
	 *   the other entries are kept empty
	 */

	if (!quiet)
	{ printf ("Filling in the contents of the root directory ... ");
		fflush (stdout);                          /* make sure the message is printed now */
	}

	if ((status = fillInRootDir (p_sb)) != 0)
	{ printError (status, basename (argv[0]));
		soCloseBufferCache ();
		return EXIT_FAILURE;
	}

	if (!quiet) printf ("done.\n");

	/*
	 * create the general repository of free data clusters as a double-linked list where the data clusters themselves are
	 * used as nodes
	 * zero fill the remaining data clusters if full formating was required:
	 *   zero mode was selected
	 */

	if (!quiet)
	{ printf ("Creating the general repository of free data clusters ... ");
		fflush (stdout);                          /* make sure the message is printed now */
	}

	if ((status = fillInGenRep (p_sb, zero)) != 0)
	{ printError (status, basename (argv[0]));
		soCloseBufferCache ();
		return EXIT_FAILURE;
	}

	if (!quiet) printf ("done.\n");

	/* magic number should now be set to the right value before writing the contents of the superblock to the storage
	   device */

	p_sb->magic = MAGIC_NUMBER;
	if ((status = soStoreSuperBlock ()) != 0)
		return status;

	/* check the consistency of the file system metadata */

	if (!quiet)
	{ printf ("Checking file system metadata... ");
		fflush (stdout);                          /* make sure the message is printed now */
	}

	if ((status = checkFSConsist ()) != 0)
	{ fprintf(stderr, "error # %d - %s\n", -status, soGetErrorMessage (p_sb, -status));
		soCloseBufferCache ();
		return EXIT_FAILURE;
	}

	if (!quiet) printf ("done.\n");

	/* close the unbuffered communication channel with the storage device */

	if ((status = soCloseBufferCache ()) != 0)
	{ printError (status, basename (argv[0]));
		return EXIT_FAILURE;
	}

	/* that's all */

	if (!quiet) printf ("Formating concluded.\n");

	return EXIT_SUCCESS;

} /* end of main */

/*
 * print help message
 */

static void printUsage (char *cmd_name)
{
	printf ("Sinopsis: %s [OPTIONS] supp-file\n"
			"  OPTIONS:\n"
			"  -n name --- set volume name (default: \"SOFS14\")\n"
			"  -i num  --- set number of inodes (default: N/8, where N = number of blocks)\n"
			"  -z      --- set zero mode (default: not zero)\n"
			"  -q      --- set quiet mode (default: not quiet)\n"
			"  -h      --- print this help\n", cmd_name);
}

/*
 * print error message
 */

static void printError (int errcode, char *cmd_name)
{
	fprintf(stderr, "%s: error #%d - %s\n", cmd_name, -errcode,
			soGetErrorMessage (soGetSuperBlock (),-errcode));
}

/* filling in the superblock fields:
 *   magic number should be set presently to 0xFFFF, this enables that if something goes wrong during formating, the
 *   device can never be mounted later on
 */

static int fillInSuperBlock (SOSuperBlock *p_sb, uint32_t ntotal, uint32_t itotal, uint32_t nclusttotal,
		unsigned char *name)
{
	p_sb->magic = 0xFFFF; /*MAGIC_NUMBER;*/
	p_sb->version = VERSION_NUMBER;

	/* inserir o nome */
	unsigned int i = 0;
	for( ; name[i]!='\0' && i<PARTITION_NAME_SIZE; i++){
		p_sb->name[i] = name[i];
	}
	p_sb->name[i] = '\0';

	p_sb->nTotal = ntotal;
	p_sb->mStat = PRU; /* formatar => foi bem desmontado */
	/*DÚVIDAS existe algum define para o valor 1? */

	/* iNodes */
	p_sb->iTableStart = 1;  /* o superbloco é o bloco 0 */
	p_sb->iTableSize = itotal / IPB; /* nº blocos da tabela de iNodes */
	p_sb->iTotal = itotal;
	p_sb->iFree = itotal - 1; /* o 1º está ocupado com a raiz */
	p_sb->iHead = 1; /* 0 => raiz */
	p_sb->iTail = itotal - 1;

	/* data clusters (cache) */
	p_sb->dZoneStart = p_sb->iTableSize + 1;
	p_sb->dZoneTotal = nclusttotal;

	/* DÚVIDAS nclusttotal - 1 por causa do . (root)? */
	for (i = 0; i < DZONE_CACHE_SIZE; ++i){
		p_sb->dZoneRetriev.cache[i] = NULL_CLUSTER;
		p_sb->dZoneInsert.cache[i] = NULL_CLUSTER;
	}


	p_sb->dZoneFree = nclusttotal - 1;
	p_sb->dZoneRetriev.cacheIdx = DZONE_CACHE_SIZE;
	p_sb->dZoneInsert.cacheIdx = 0;

	/* data clusters */
	p_sb->dHead = 1; /* 0 é a raiz, root */
	p_sb->dTail = nclusttotal - 1;

	for (i = 0; i < RESERV_AREA_SIZE; ++i){
		p_sb->reserved[i] = 0xee;
	}

	return 0;
}

/*
 * filling in the inode table:
 *   only inode 0 is in use (it describes the root directory)
 */

static int fillInINT (SOSuperBlock *p_sb)
{
	int stat;

	/*preenchimento do 1º iNode*/
	if((stat = soLoadBlockInT(0)) != 0) return stat; /* Load the contents of a specific block of the table of inodes into internal storage. Portanto é preciso ir buscar o SOInode, faz-se em baixo*/

	SOInode* pToBckInode = soGetBlockInT(); /* Get a pointer to the contents of a specific block of the table of inodes. */

	/*definir as permissões de acesso ao owner, group e other*/
	pToBckInode->mode = 0x01ff | INODE_DIR;

	/*número de directory entries associadas ao iNode, neste caso 2, "." e ".."*/
	pToBckInode->refCount = 2;

	/*obter os id do owner e do group*/
	pToBckInode->owner = getuid();
	pToBckInode->group = getgid();

	/*size in bytes*/
	/* REVIEW: não tenho a certeza que seja assim */
	pToBckInode->size = DPC*sizeof(SODirEntry);/*numero maximo de directorios por cluster, o cluster fica formatado a estas entradas, sendo que as duas primeiras ficam em uso*/

	/*size in clusters*/
	pToBckInode->cluCount = 1;

	/*como está ocupado o iNode, então as union vD1 e vD2, ficam reservadas ao tempo.*/
	pToBckInode->vD1.aTime = time(NULL);
	pToBckInode->vD2.mTime = time(NULL);

	pToBckInode->d[0] = p_sb->dZoneStart;

	/*referencias a clusters a NULL*/
	int i;
	for(i = 1; i < N_DIRECT; i++){
		pToBckInode->d[i] = NULL_CLUSTER;
	}

	pToBckInode->i1 = NULL_CLUSTER;
	pToBckInode->i2 = NULL_CLUSTER;

	/* Store the contents of the block of the table of inodes resident in internal storage to the storage device.*/
	if((stat=soStoreBlockInT())!=0){
		return stat;
	}

	/* Inicializar os Free iNodes */
	/* Vamos obter bloco a bloco apartir do bloco 0, temos de ter atenção que existem x inodes por bloco e já vamos usar um inode do bloco 0, esse vamos ter de o passar à frente */
	int idxBckOfInodes, idxInodesOfBck;
	for(idxBckOfInodes = 0; idxBckOfInodes < p_sb->iTableSize; idxBckOfInodes++){
		/* se estivermos no bloco 0, temos de fazer com que o for apenas comece no inode 1*/
		if(idxBckOfInodes == 0){
			idxInodesOfBck = 1;
		}else{
			idxInodesOfBck = 0;
		}

		/* vamos obter o bloco de inodes (InodesPerBlock, não se esqueçam!!!)*/
		if((stat = soLoadBlockInT(idxBckOfInodes)) != 0){
			return stat;
		}

		/* obter o bloco de inodes que está armazenado internamente */
		pToBckInode = soGetBlockInT();

		/* vamos percorrer o bloco de inodes, desde o inicial, 0 ou 1 até ao final (IPB) */
		for(; idxInodesOfBck < IPB; idxInodesOfBck++){
			pToBckInode[idxInodesOfBck].mode = INODE_FREE;
			pToBckInode[idxInodesOfBck].refCount = 0;
			pToBckInode[idxInodesOfBck].owner = 0;
			pToBckInode[idxInodesOfBck].group = 0;
			pToBckInode[idxInodesOfBck].cluCount = 0;
			pToBckInode[idxInodesOfBck].size = 0;

			/*inicializar as referecias a cluster a NULL*/
			for(i=0; i<N_DIRECT; i++){
				pToBckInode[idxInodesOfBck].d[i] = NULL_CLUSTER;
			}

			pToBckInode[idxInodesOfBck].i1 = NULL_CLUSTER;
			pToBckInode[idxInodesOfBck].i2 = NULL_CLUSTER;

			/*se estivermos no ultimo iNode da tabela, entao o seguinte é NULL*/
			if(idxBckOfInodes == p_sb->iTableSize - 1 && idxInodesOfBck == IPB - 1){
				pToBckInode[idxInodesOfBck].vD1.next = NULL_INODE;
			}else{
				/*caso não seja o ultimo então incrementamos 1 no numero do iNode actual*/
				pToBckInode[idxInodesOfBck].vD1.next = idxBckOfInodes * IPB + idxInodesOfBck + 1;
			}

			/* Se estivermos no iNode 1 do primeiro bloco, então o anterior é NULL*/
			if(idxBckOfInodes == 0 && idxInodesOfBck == 1){
				pToBckInode[idxInodesOfBck].vD2.prev = NULL_INODE;
			}else{
				/*caso não seja o iNode 1 do primeiro bloco, decrementa-se 2 ao next*/
				pToBckInode[idxInodesOfBck].vD2.prev = pToBckInode[idxInodesOfBck].vD1.next - 2;
			}
		}

		if((stat = soStoreBlockInT()) != 0){
			return stat;
		}
	}

	return 0;
}

/*
 * filling in the contents of the root directory:
 the first 2 entries are filled in with "." and ".." references
 the other entries are empty
 */

static int fillInRootDir (SOSuperBlock *p_sb)
{

	/* temos de começar por criar um cluster da zona de dados que será o 0 neste caso, vai conter a informação da raiz */
	SODataClust rootCluster;
	/* o soDataClust tem 3 campos: prev, next, stat e info */
	rootCluster.prev = NULL_CLUSTER;
	rootCluster.next = NULL_CLUSTER;
	rootCluster.stat = 0; /* status of the data cluster */

	/* terá informação guardada,que é um conjunto de entradas de directórios */
	/* O cluster vai ter DPC entradas de diretório, das quais a 1º será a ., a 2ª a .. e as restantes estarão vazias. */
	/*
	   typedef struct soDirEntry{
	   unsigned char name[MAX_NAME+1];
	   uint32_t nInode;
	   } SODirEntry;
	 */
	SODirEntry root1 = {".\0", 0};
	SODirEntry root2 = {"..\0", 0};
	SODirEntry emptyDir = {"\0", NULL_INODE};

	rootCluster.info.de[0] = root1;
	rootCluster.info.de[1] = root2;

	int i = 2;
	for (; i < DPC; ++i){
		rootCluster.info.de[i] = emptyDir;
	}

	/* depois temos de escrever o soDataClust.. */
	/* soWriteCacheCluster => sofs_buffercache.h */
	return soWriteCacheCluster(p_sb->dZoneStart, &rootCluster);
}

/*
 * create the general repository of free data clusters as a double-linked list where the data clusters themselves are
 * used as nodes
 * zero fill the remaining data clusters if full formating was required:
 *   zero mode was selected
 */

static int fillInGenRep (SOSuperBlock *p_sb, int zero)
{

	/* Criar a ligação entre clusters (lista ligada) */

	SODataClust c;	// Criação de um ponteiro para cluster de dados
	uint32_t i;		// variavel usada para correr a zona de dados

	for(i = (p_sb->dZoneStart + BLOCKS_PER_CLUSTER) ; i < (p_sb->dZoneStart + (p_sb->dZoneTotal * BLOCKS_PER_CLUSTER)) ; i += BLOCKS_PER_CLUSTER)
	{

		if(i == (p_sb->dZoneStart + BLOCKS_PER_CLUSTER)) // O primeiro cluster não possui nenhum antes
		{
			c.prev = NULL_CLUSTER;
			c.next = i+BLOCKS_PER_CLUSTER;

		}else if (i>(p_sb->dZoneStart + BLOCKS_PER_CLUSTER) && i<(p_sb->dZoneStart + (p_sb->dZoneTotal-1 * BLOCKS_PER_CLUSTER))) // restantes clusters
		{
			c.prev = i-BLOCKS_PER_CLUSTER;
			c.next = i+BLOCKS_PER_CLUSTER;

		}else                                          // O último cluster não aponta para nada
		{
			c.prev = i-BLOCKS_PER_CLUSTER;
			c.next = NULL_CLUSTER;	
		}

		if(soWriteCacheCluster(i,&c) != 0)				// Armazena informação
		{
			return soWriteCacheCluster(i,&c);
		}
	}

	/* Zero mode */
	if (zero)   		// Teste da flag
	{
		memset(&c,0x00,BSLPC); // Conteúdo informativo do cluster a zero

		for(i = (p_sb->dZoneStart + BLOCKS_PER_CLUSTER) ; i < (p_sb->dZoneStart + (p_sb->dZoneTotal * BLOCKS_PER_CLUSTER)) ; i += BLOCKS_PER_CLUSTER)
		{
			if(soWriteCacheCluster(i,&c) != 0)		 // Armazena informação
			{
				return soWriteCacheCluster(i,&c);
			}
		}
	}


	return 0;

}

/*
   check the consistency of the file system metadata
 */

static int checkFSConsist (void)
{
	SOSuperBlock *p_sb;                            /* pointer to the superblock */
	SOInode *inode;                                /* pointer to the contents of a block of the inode table */
	int stat;                                      /* status of operation */

	/* read the contents of the superblock to the internal storage area and get a pointer to it */

	if ((stat = soLoadSuperBlock ()) != 0) return stat;
	p_sb = soGetSuperBlock ();

	/* check superblock and related structures */

	if ((stat = soQCheckSuperBlock (p_sb)) != 0) return stat;

	/* read the contents of the first block of the inode table to the internal storage area and get a pointer to it */

	if ((stat = soLoadBlockInT (0)) != 0) return stat;
	inode = soGetBlockInT ();

	/* check inode associated with root directory (inode 0) and the contents of the root directory */

	if ((stat = soQCheckInodeIU (p_sb, &inode[0])) != 0) return stat;
	if ((stat = soQCheckDirCont (p_sb, &inode[0])) != 0) return stat;

	/* everything is consistent */

	return 0;
}
