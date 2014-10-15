/**
 *  \file testifuncs14.c (implementation file)
 *
 *  \brief The SOFS14 internal testing tool.
 *
 *  It provides a simple method to test separately the file system internal operations.
 *
 *  Level 1 - Management of the double-linked lists of free inodes and free data clusters:
 *     \li allocate a free inode
 *     \li free the referenced inode
 *     \li allocate a free data cluster
 *     \li free the referenced data cluster.
 *
 *  Level 2 - Management of inodes:
 *     \li read specific inode data from the table of inodes
 *     \li write specific inode data to the table of inodes
 *     \li clean an inode
 *     \li check the inode access permissions against a given operation.
 *
 *  Level 3 - Management of data clusters:
 *     \li read a specific data cluster
 *     \li write to a specific data cluster
 *     \li handle a file data cluster
 *     \li free and clean all data clusters from the list of references starting at a given point
 *     \li clean a data cluster from the inode describing a file which was previously deleted.
 *
 *  Level 4 - Management of directories and directory entries:
 *      \li get an entry by path
 *      \li get an entry by name
 *      \li add an new entry / attach a directory entry to a directory
 *      \li remove an entry / detach a directory entry from a directory
 *      \li rename an entry of a directory
 *      \li check a directory status of emptiness.
 *
 *  SINOPSIS:
 *  <P><PRE>                testifuncs14 [OPTIONS] supp-file

                  OPTIONS:
                   -b       --- set batch mode (default: not batch)
                   -l depth --- set log depth (default: 0,0)
                   -L file  --- log file (default: stdout)
                   -h       --- print this help.</PRE>
 *
 *  \author Artur Carneiro Pereira - October 2005
 *  \author Miguel Oliveira e Silva - September 2009
 *  \author António Rui Borges - October 2010 / September 2011, September 2014
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "sofs_probe.h"
#include "sofs_const.h"
#include "sofs_buffercache.h"
#include "sofs_superblock.h"
#include "sofs_inode.h"
#include "sofs_direntry.h"
#include "sofs_datacluster.h"
#include "sofs_basicoper.h"
#include "sofs_basicconsist.h"
#include "sofs_ifuncs_1.h"
#define  IFUNCS_2
#ifdef IFUNCS_2
#include "sofs_ifuncs_2.h"
#endif
#define  IFUNCS_3
#ifdef IFUNCS_3
#include "sofs_ifuncs_3.h"
#endif
/* #define  IFUNCS_4 */
#ifdef IFUNCS_4
#include "sofs_ifuncs_4.h"
#endif

/* Allusion to internal functions */

static void printMenu (void);
static void printUsage (char *cmd_name);
static void printError (int errcode, char *cmd_name);
#ifdef IFUNCS_2
static void printInode (SOInode *p_inode, uint32_t nInode);
#endif
#ifdef IFUNCS_3
static void printCluster (SODataClust *clust, uint32_t nClust);
#endif
static void notUsed (void);
static void neverCalled (void);
static void allocInode (void);
static void freeInode (void);
static void allocDataCluster (void);
static void freeDataCluster (void);
#ifdef IFUNCS_2
static void readInode (void);
static void writeInode (void);
static void cleanInode (void);
static void accessGranted (void);
#endif
#ifdef IFUNCS_3
static void readFileCluster (void);
static void writeFileCluster (void);
static void handleFileCluster (void);
static void handleFileClusters (void);
static void cleanDataCluster (void);
#endif
#ifdef IFUNCS_4
static void getDirEntryByPath (void);
static void getDirEntryByName (void);
static void addAttachDirEntry (void);
static void removeDetachDirEntry (void);
static void renameDirEntry (void);
static void checkDirectoryEmptiness (void);
static void initSymLink (void);
#endif

/* Definition of the handler functions */

typedef void (*handler)(void);

static handler hdl[] = { neverCalled,            /* 0 */
                         allocInode,             /* 1 */
                         freeInode,              /* 2 */
                         allocDataCluster,       /* 3 */
                         freeDataCluster,        /* 4 */
#ifdef IFUNCS_2
                         readInode,              /* 5 */
                         writeInode,             /* 6 */
                         cleanInode,             /* 7 */
                         accessGranted,          /* 8 */
#endif
#ifdef IFUNCS_3
                         readFileCluster,        /* 9 */
                         writeFileCluster,       /* 10 */
                         handleFileCluster,      /* 11 */
                         handleFileClusters,     /* 12 */
                         cleanDataCluster,       /* 13 */
#endif
#ifdef IFUNCS_4
                         getDirEntryByPath,      /* 14 */
                         getDirEntryByName,      /* 15 */
                         addAttachDirEntry,      /* 16 */
                         removeDetachDirEntry,   /* 17 */
                         renameDirEntry,         /* 18 */
                         checkDirectoryEmptiness,/* 19 */
                         initSymLink             /* 20 */
#endif
                       };

#define HDL_LEN (sizeof (hdl) / sizeof (handler))

/* Batch mode flag */

static int batch = 0;                                 /* if kept set not batch mode */

/* Bit pattern description of mode field in inode data type */

#ifdef IFUNCS_2
static const char *inodetypes[] = { "INVALID-0000",
                                    "symlink",
                                    "file",
                                    "INVALID-0011",
                                    "dir",
                                    "INVALID-0101",
                                    "INVALID-0110",
                                    "INVALID-0111",
                                    "empty and clean",
                                    "deleted symlink",
                                    "deleted file",
                                    "INVALID-1011",
                                    "deleted dir",
                                    "INVALID-1101",
                                    "INVALID-1110",
                                    "INVALID-1111"
                                  };
#endif

/* Log stream id */

static FILE *fl = NULL;                               /* log stream default */

/* The main function */

int main (int argc, char *argv[])
{
  int lower = 0;                                 /* lower limit of log depth, if kept set to zero */
  int higher = 0;                                /* upper limit of log depth, if kept set to zero */

  /* process command line options */

  int opt;                                       /* selected option */

  do
  { switch ((opt = getopt (argc, argv, "l:L:bh")))
    { case 'l': /* log depth */
                if (sscanf (optarg, "%d,%d", &lower, &higher) != 2)
                   { fprintf (stderr, "%s: Bad argument to l option.\n", basename (argv[0]));
                     printUsage (basename (argv[0]));
                     return EXIT_FAILURE;
                }
                soSetProbe (lower, higher);
                break;
      case 'L': /* log file */
                if ((fl = fopen (optarg, "w")) == NULL)
                   { fprintf (stderr, "%s: Can't open log file \"%s\".\n", basename (argv[0]), optarg);
                     printUsage (basename (argv[0]));
                     return EXIT_FAILURE;
                   }
                soOpenProbe (fl);
                break;
      case 'b': /* batch mode */
                batch = 1;                       /* set batch mode for processing: no input messages are issued */
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
  if (fl == NULL)
     fl = stdout;                                /* if the switch -L was not used, set output to stdout */
     else stderr = fl;                           /* if the switch -L was used, set stderr to log file */

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

  /* open an unbuffered communication channel with the storage device */

  int status;                                    /* status of operation */

  if ((status = soOpenBufferCache (argv[optind], UNBUF)) != 0)
     { printError (status, basename (argv[0]));
       return EXIT_FAILURE;
     }

  /* process the command */

  int cmdNumb;                                   /* command number */
  int t;                                         /* test flag */

  while (true)
  { if (batch == 0) printMenu ();
    if (batch == 0) printf("\nYour command: ");
    do
    { t = scanf ("%d", &cmdNumb);
      scanf ("%*[^\n]");
      scanf ("%*c");
    } while (t != 1);

  if (cmdNumb == 0) break;
  if ((cmdNumb > 0) && (cmdNumb < HDL_LEN))
     hdl[cmdNumb]();
     else { notUsed();
            if (batch != 0) break;
          }
  }


  /* close the unbuffered communication channel with the storage device */

  if ((status = soCloseBufferCache ()) != 0)
     { printError (status, basename (argv[0]));
       return EXIT_FAILURE;
     }

  /* that's all */

  if (batch == 0) printf ("Bye!\n");

  return EXIT_SUCCESS;

} /* end of main */

/*
 * print help message
 */

static void printUsage (char *cmd_name)
{
  printf ("Sinopsis: %s [OPTIONS] supp-file\n"
          "  OPTIONS:\n"
          "  -b       --- set batch mode (default: not batch)\n"
          "  -l depth --- set log depth (default: 0,0)\n"
          "  -L file  --- log file (default: stdout)\n"
          "  -h       --- print this help\n", cmd_name);
}

/*
 * print error message
 */

static void printError (int errcode, char *cmd_name)
{
  fprintf(stderr, "%s: error #%d - %s\n", cmd_name, -errcode,
          soGetErrorMessage (soGetSuperBlock (), -errcode));
}

/*
 * print menu
 */

static void printMenu (void)
{
  printf(
               "+==============================================================+\n"
               "|                      IFuncs testing tool                     |\n"
               "+==============================================================+\n"
               "|  0 - exit                                                    |\n"
               "+--------------------------------------------------------------+\n"
               "|  1 - soAllocInode            2 - soFreeInode                 |\n"
               "|  3 - soAllocDataCluster      4 - soFreeDataCluster           |\n");
#ifdef IFUNCS_2
  printf(
               "+--------------------------------------------------------------+\n"
               "|  5 - soReadInode             6 - soWriteInode                |\n"
               "|  7 - soCleanInode            8 - soAccessGranted             |\n");
#endif
#ifdef IFUNCS_3
  printf(
               "+--------------------------------------------------------------+\n"
               "|  9 - soReadFileCluster      10 - soWriteFileCluster          |\n"
               "| 11 - soHandleFileCluster    12 - soHandleFileClusters        |\n"
               "| 13 - soCleanDataCluster                                      |\n");
#endif
#ifdef IFUNCS_4
  printf(
               "+--------------------------------------------------------------+\n"
               "| 14 - soGetDirEntryByPath    15 - soGetDirEntryByName         |\n"
               "| 16 - soAddAttachDirEntry    17 - soRemoveDetachDirEntry      |\n"
               "| 18 - soRenameDirEntry       19 - soCheckDirectoryEmptiness   |\n"
               "+--------------------------------------------------------------+\n"
               "| 20 - soInitSymLink                                           |\n");
#endif
  printf(
               "+==============================================================+\n");
}

/*
 * not used
 */

static void notUsed (void)
{
  fprintf (stderr, "\e[02;41m==>\e[0m ");
  fprintf (stderr, "Invalid option. Try again!\n");
}

/*
 * never called
 */

static void neverCalled (void)
{
}

/*
 * alloc inode
 */

static void allocInode (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  uint32_t mode[3] =                             /* allowed type values */
              {INODE_DIR,INODE_FILE,INODE_SYMLINK};
  uint32_t type;                                 /* file type */
  uint32_t nInode;                               /* inode number */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Alloc Inode\n");
  if (batch == 0) printf("Inode type (1 - dir, 2 - file, 3 - symlink): ");
  do
  { t = scanf ("%d", &valInt);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  type = (((valInt >= 1) && (valInt <=3)) ? mode[valInt-1] : 0);
  if ((stat = soAllocInode (type, &nInode)) != 0)
     printError (stat, "soAllocInode");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            fprintf (fl, "Inode no. %u alocated.\n", nInode);
          }
}

/*
 * free inode
 */

static void freeInode (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  uint32_t nInode;                               /* inode number */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Free Inode\n");
  if (batch == 0) printf("Inode number: ");
  do
  { t = scanf ("%d", &valInt);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  nInode = (uint32_t) valInt;
  if ((stat = soFreeInode (nInode)) != 0)
     printError (stat, "soFreeInode");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            fprintf (fl, "Inode no. %u freed.\n", nInode);
          }
}

/*
 * alloc data cluster
 */

static void allocDataCluster (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  uint32_t nInode;                               /* inode number */
  uint32_t nClust;                               /* logical cluster number */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Alloc Data Cluster\n");
  if (batch == 0) printf("Inode number: ");
  do
  { t = scanf ("%d", &valInt);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  nInode = (uint32_t) valInt;
  if ((stat = soAllocDataCluster (nInode, &nClust)) != 0)
     printError (stat, "soAllocDataCluster");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            fprintf (fl, "Cluster no. %u alocated.\n", nClust);
          }
}

/*
 * free file cluster
 */

static void freeDataCluster (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  uint32_t nClust;                               /* physical cluster number */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Free Data Cluster\n");
  if (batch == 0) printf("Logical cluster number: ");
  do
  { t = scanf ("%d", &valInt);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  nClust = (uint32_t) valInt;
  if ((stat = soFreeDataCluster (nClust)) != 0)
     printError (stat, "soFreeFileCluster");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            fprintf (fl, "Cluster no. %u freed.\n", nClust);
          }
}

#ifdef IFUNCS_2

/*
 * read inode
 */

static void readInode (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  uint32_t nInode;                               /* inode number */
  SOInode inode;                                 /* inode */
  uint32_t status;                               /* inode status */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Read Inode\n");
  if (batch == 0) printf("Inode number: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nInode = (uint32_t) valInt;
  if (batch == 0) printf("Inode status (in use = 0, free in dirty state = 1): ");
  do
  { t = scanf ("%d", &valInt);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  status = (uint32_t) valInt;
  if ((stat = soReadInode (&inode, nInode, status)) != 0)
     printError (stat, "soReadInode");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            printInode (&inode, nInode);
          }
}

/*
 * print inode
 */

static void printInode (SOInode *p_inode, uint32_t nInode)
{
  char perm[10] = "rwxrwxrwx";                   /* access rights array */
  int i;                                         /* counting variable */
  int m;                                         /* mask variable */
  time_t temp;                                   /* time indication in seconds */
  char timebuf[30];                              /* date and time string */

  fprintf (fl, "Inode #");
  if (nInode == NULL_INODE)
     fprintf (fl, "(nil)\n");
     else fprintf (fl, "%"PRIu32"\n", nInode);

  fprintf (fl, "type = %s, ", inodetypes[(int)((p_inode->mode & (INODE_FREE | INODE_TYPE_MASK)) >> 9)]);
  for (i = 8, m = 1; i >= 0; i--, m <<= 1)
    if ((p_inode->mode & m) == 0) perm[i] = '-';
  fprintf (fl, "permissions = %s, ", perm);
  fprintf (fl, "refcnt = %"PRIu16", ", p_inode->refCount);
  fprintf (fl, "owner = %"PRIu32", group = %"PRIu32"\n", p_inode->owner, p_inode->group);
  fprintf (fl, "size in bytes = %"PRIu32", size in clusters = %"PRIu32"\n", p_inode->size, p_inode->cluCount);
  if (p_inode->mode & INODE_FREE)
     /* inode is free */
     { fprintf (fl, "prev = ");
       if (p_inode->vD2.prev == NULL_INODE)
           fprintf (fl, "(nil), ");
           else fprintf (fl, "%"PRIu32", ", p_inode->vD2.prev);
       fprintf (fl, "next = ");
       if (p_inode->vD1.next == NULL_INODE)
           fprintf (fl, "(nil)\n");
           else fprintf (fl, "%"PRIu32"\n", p_inode->vD1.next);
     }
  else /* inode is in use */
       { temp = p_inode->vD1.aTime;
         ctime_r (&temp, timebuf);
         timebuf[strlen (timebuf) - 1] = '\0';
         fprintf (fl, "atime = %s, ", timebuf);
         temp = p_inode->vD2.mTime;
         ctime_r (&temp, timebuf);
         timebuf[strlen (timebuf) - 1] = '\0';
         fprintf (fl, "mtime = %s\n", timebuf);
       }
  fprintf (fl, "d[] = {");
  for (i = 0; i < N_DIRECT; i++)
  { if (i > 0) fprintf (fl, "%c", ' ');
    if (p_inode->d[i] == NULL_CLUSTER)
       fprintf (fl, "(nil)");
       else fprintf (fl, "%"PRIu32"", p_inode->d[i]);
  }
  fprintf (fl, "}, ");
  fprintf (fl, "i1 = ");
  if (p_inode->i1 == NULL_CLUSTER)
     fprintf (fl, "(nil), ");
     else fprintf (fl, "%"PRIu32", ", p_inode->i1);
  fprintf (fl, "i2 = ");
  if (p_inode->i2 == NULL_CLUSTER)
     fprintf (fl, "(nil)\n");
     else fprintf (fl, "%"PRIu32"\n", p_inode->i2);
  fprintf (fl, "----------------\n");
}

/*
 * write inode
 */

static void writeInode (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  SOSuperBlock *p_sb;                            /* pointer to the region where the superblock is stored */
  uint32_t nInode;                               /* inode number */
  SOInode inode;                                 /* inode */
  uint32_t status;                               /* inode status */
  SOInode *in;                                   /* pointer to the contents of a block of the inode table */
  uint32_t nBlk;                                 /* block number of the inode table it belongs to */
  uint32_t offset;                               /* index of the inode within the block of the inode table it
                                                    belongs to */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Write Inode\n");
  if (batch == 0) printf("Inode number: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nInode = (uint32_t) valInt;
  if (batch == 0) printf("Inode status (in use = 0, free in dirty state = 1): ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  status = (uint32_t) valInt;
  if ((stat = soLoadSuperBlock ()) != 0)
     { printError (stat, "soWriteInode");
       return;
     }
  p_sb = soGetSuperBlock ();
  if (nInode < p_sb->iTotal)
     { if ((stat = soConvertRefInT (nInode, &nBlk, &offset)) != 0)
          { printError (stat, "soWriteInode");
            return;
          }
       if ((stat = soLoadBlockInT (nBlk)) != 0)
          { printError (stat, "soWriteInode");
            return;
          }
       in = soGetBlockInT ();
       inode = in[offset];
     }
     else inode.mode = INODE_FILE;
  inode.owner = getuid();
  inode.group = getgid();
  if (batch == 0) printf ("Inode permission (a value in octal): ");
  do
  { t = scanf ("%o", &valInt);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  inode.mode &= (uint16_t) 0xfe00;
  inode.mode |= (uint16_t) valInt;
  if ((stat = soWriteInode (&inode, nInode, status)) != 0)
     printError (stat, "soWriteInode");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            fprintf(fl, "Inode no. %u successfully written.\n", nInode);
          }
}

/*
 * clean inode
 */

static void cleanInode (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  uint32_t nInode;                               /* inode number */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Clean Inode\n");
  if (batch == 0) printf("Inode number: ");
  do
  { t = scanf ("%d", &valInt);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  nInode = (uint32_t) valInt;
  if ((stat = soCleanInode (nInode)) != 0)
     printError (stat, "soCleanInode");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            fprintf(fl, "Inode %u successfully cleaned.\n", nInode);
          }
}

/*
 * access granted
 */

static void accessGranted (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  uint32_t nInode;                               /* inode number */
  uint32_t opRequested;                          /* requested operation */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Access Granted\n");
  if (batch == 0) printf("Inode number: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nInode = (uint32_t) valInt;
  if ((stat = soLoadSuperBlock ()) != 0)
     { printError (stat, "soWriteInode");
       return;
     }
  if (batch == 0) printf("Requested operation (R = 4, W = 2, X = 1): ");
  do
  { t = scanf ("%d", &valInt);
    if (t == 0)
       { t = 1;
         valInt = 0;
       }
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  opRequested = (uint32_t) valInt;
  if (((stat = soAccessGranted (nInode, opRequested)) != 0) && (stat != -EACCES))
     printError (stat, "soAccessGranted");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            if (stat == 0)
               fprintf(fl, "Access to inode %u is granted.\n", nInode);
               else fprintf(fl, "Access to inode %u is not granted.\n", nInode);
          }
}

#endif

#ifdef IFUNCS_3
/*
 * read file cluster
 */

static void readFileCluster (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  uint32_t nInode;                               /* inode number */
  uint32_t nClust;                               /* index to the list of direct references */
  SODataClust dc;                                /* data cluster buffer */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Read File Cluster\n");
  if (batch == 0) printf("Inode number: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nInode = (uint32_t) valInt;
  if (batch == 0) printf("Index to the list of direct references: ");
  do
  { t = scanf ("%d", &valInt);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  nClust = (uint32_t) valInt;
  if ((stat = soReadFileCluster (nInode, nClust, &dc)) != 0)
     printError (stat, "soReadFileCluster");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            printCluster (&dc, nClust);
          }
}

/*
 * print cluster
 */

static void printCluster (SODataClust *clust, uint32_t nClust)
{
  fprintf (fl, "Index to the list of direct references number ");
  if (nClust == NULL_CLUSTER)
     fprintf (fl, "(nil)\n");
     else fprintf (fl, "%"PRIu32"\n", nClust);

  unsigned char *c;                              /* pointer to a character array */
  char line[256];                                /* line to be printed */
  char *p_line = line;                           /* pointer to a character in the line */
  int i;                                         /* counting variable */

  c = (unsigned char *) clust->info.data;

  for (i = 0; i < BSLPC; i++)
  { /* print address label and initialize pointer, if required */
    if ((i & 0x0f) == 0)
       { fprintf (fl, "%4.4x: ", i);
         p_line = line;
       }
    /* print character and add it to the line */
    fprintf (fl, " %.2x", c[i]);
    if ((i & 0x0f) == 0x0) p_line += sprintf (p_line, "    ");
    if (i == (BSLPC - 4)) p_line += sprintf (p_line, "                                    ");
    switch (c[i])
    { case '\a': p_line += sprintf (p_line, " \\a");
                 break;
      case '\b': p_line += sprintf (p_line, " \\b");
                 break;
      case '\f': p_line += sprintf (p_line, " \\f");
                 break;
      case '\n': p_line += sprintf (p_line, " \\n");
                 break;
      case '\r': p_line += sprintf (p_line, " \\r");
                 break;
      case '\t': p_line += sprintf (p_line, " \\t");
                 break;
      case '\v': p_line += sprintf (p_line, " \\v");
                 break;
      default:  if ((c[i] >= ' ') && (c[i] != 0x7F) && (c[i] != 0x8F))
                   p_line += sprintf (p_line, " %c ", c[i]);
                   else p_line += sprintf (p_line, " %.2x", c[i]);
                /* no break is required */
    }
    /* terminate and print present line, if required */
    if (((i & 0x0f) == 0x0f) || (i == (BSLPC - 1)))
       { *p_line = '\0';
         fprintf (fl, "%s\n", line);
       }
  }
}

/*
 * write file cluster
 */

static void writeFileCluster (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  uint32_t nInode;                               /* inode number */
  uint32_t nClust;                               /* index to the list of direct references */
  uint8_t byte;                                  /* character to be written in the cluster */
  SODataClust dc;                                /* data cluster buffer */
  int stat;                                      /* status of operation */
  int i;                                         /* counting variable */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Write File Cluster\n");
  if (batch == 0) printf("Inode number: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nInode = (uint32_t) valInt;
  if (batch == 0) printf("Index to the list of direct references: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nClust = (uint32_t) valInt;
  if (batch == 0) printf("Character to be written in the cluster: ");
  do
  { t = scanf ("%x", &valInt);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  byte = (uint8_t) valInt;
  for (i = 0; i < BSLPC; i++)
    dc.info.data[i] = byte;
  if ((stat = soWriteFileCluster (nInode, nClust, &dc)) != 0)
     printError (stat, "soWriteFileCluster");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            fprintf(fl, "Cluster with index no. %u  to the list of direct references is successfully written.\n", nClust);
          }
}

/*
 * handle file cluster
 */

static void handleFileCluster (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  uint32_t nInode;                               /* inode number */
  uint32_t nClust;                               /* index to the list of direct references */
  uint32_t npClust;                              /* logical cluster number */
  uint32_t op;                                   /* operation to be performed */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Handle File Cluster\n");
  if (batch == 0) printf("Inode number: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nInode = (uint32_t) valInt;
  if (batch == 0) printf("Index to the list of direct references: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nClust = (uint32_t) valInt;
  if (batch == 0)
     { printf ("Operation to be performed:\n");
       printf ("    0 - get the logical number (or reference) of the referred data cluster\n");
       printf ("    1 - allocate a new data cluster and include it in the list of references\n");
       printf ("        of the inode which describes the file\n");
       printf ("    2 - free the referred data cluster and dissociate it from the list of references\n");
       printf ("        of the inode which describes the file\n");
       printf ("What is your option? ");
     }
  do
  { t = scanf ("%x", &valInt);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  op = (uint32_t) valInt;
  if (op < 2)
     stat = soHandleFileCluster (nInode, nClust, op, &npClust);
     else stat = soHandleFileCluster (nInode, nClust, op, NULL);
  if (stat != 0)
     printError (stat, "soHandleFileCluster");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            switch (op)
            { case 0:  if (npClust == NULL_CLUSTER)
                          fprintf(fl, "Logical cluster whose index to the list of direct references is %u, is (nil).\n",
                                  nClust);
                          else fprintf(fl, "Logical cluster whose index to the list of direct references is %u, is %u.\n",
                                       nClust, npClust);
                       break;
              case 1:  fprintf(fl, "Logical cluster no. %u is successfully allocated.\n", npClust);
                       break;
              case 2:  fprintf(fl, "Cluster whose index to the list of direct references is %u, is successfully freed.\n",
                               nClust);
                       break;
              case 3:  fprintf(fl, "Cluster whose index to the list of direct references is %u successfully freed and cleaned.\n",
                               nClust);
                       break;
              case 4:  fprintf(fl, "Cluster whose index to the list of direct references is %u successfully cleaned.\n",
                               nClust);
                       break;
              default: break;
            }
          }
}

/*
 * handle file clusters
 */

static void handleFileClusters (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  uint32_t nInode;                               /* inode number */
  uint32_t nClust;                               /* index to the list of direct references */
  uint32_t op;                                   /* operation to be performed */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Handle File Clusters\n");
  if (batch == 0) printf("Inode number: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nInode = (uint32_t) valInt;
  if (batch == 0) printf("Number of initial index to the list of direct references: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nClust = (uint32_t) valInt;
  if (batch == 0)
     { printf("Operation to be performed:\n");
       printf("    2 - free the referenced data cluster\n");
       printf("    3 - free the referenced data cluster and dissociate it from the inode which describes the file\n");
       printf("    4 - dissociate the referenced data cluster from the inode which describes the file\n");
       printf("What is your option? ");
     }
  do
  { t = scanf ("%x", &valInt);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  op = (uint32_t) valInt;
  stat = soHandleFileClusters (nInode, nClust, op);
  if (stat != 0)
     printError (stat, "soHandleFileClusters");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            switch (op)
            { case 2:  fprintf(fl, "All clusters starting at index %u to the list of direct references successfully freed.\n",
                               nClust);
                       break;
              case 3:  fprintf(fl, "All clusters starting at index %u to the list of direct references successfully free and cleaned.\n",
                               nClust);
                       break;
              case 4:  fprintf(fl, "All clusters starting at index %u to the list of direct references successfully cleaned.\n",
                       nClust);
                       break;
              default: break;
            }
          }
}

/*
 * clean data cluster
 */

static void cleanDataCluster (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  uint32_t nInode;                               /* inode number */
  uint32_t nClust;                               /* logical cluster number */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Clean Data Cluster\n");
  if (batch == 0) printf("Inode number: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nInode = (uint32_t) valInt;
  if (batch == 0) printf("Cluster number: ");
  do
  { t = scanf ("%d", &valInt);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  nClust = (uint32_t) valInt;
  if ((stat = soCleanDataCluster (nInode, nClust)) != 0)
     printError (stat, "soCleanDataCluster");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            fprintf (fl, "Cluster no. %u cleaned.\n", nClust);
          }
}
#endif

#ifdef IFUNCS_4
/*
 * get directory entry by path
 */

static void getDirEntryByPath (void)
{
  int t;                                         /* test flag */
  char path[MAX_PATH+1];                         /* path */
  uint32_t nInodeDir;                            /* number of the directory inode */
  uint32_t nInodeEnt;                            /* number of the entry inode */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Get Directory Entry by Path\n");
  if (batch == 0) printf("Path: ");
  do
  { t = scanf ("%255s", path);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  stat = soGetDirEntryByPath (path, &nInodeDir, &nInodeEnt);
  if (stat != 0)
     printError (stat, "soGetDirEntryByPath");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            fprintf(fl, "The entry has inode no. %u and its parent directory has inode no. %u.\n",
                    nInodeEnt, nInodeDir);
          }
}

/*
 * get directory entry by name
 */

static void getDirEntryByName (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  char name[MAX_NAME+1];                         /* entry name */
  uint32_t nInodeDir;                            /* number of the directory inode */
  uint32_t nInodeEnt;                            /* number of the entry inode */
  uint32_t idx;                                  /* index to a direntry of the directory contents seen as an array of
                                                    direntries */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Get Directory Entry by Name\n");
  if (batch == 0) printf("Inode number of the directory: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nInodeDir = (uint32_t) valInt;
  if (batch == 0) printf("Name of the entry: ");
  do
  { t = scanf ("%59s", name);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  stat = soGetDirEntryByName (nInodeDir, name, &nInodeEnt, &idx);
  if (stat != 0)
     printError (stat, "soGetDirEntryByName");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            fprintf(fl, "The entry has name %s and inode no. %u and is the entry no. %u in the parent directory.\n",
                    name, nInodeEnt, idx);
          }
}

/*
 * add a directory entry
 */

static void addAttachDirEntry (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  char name[MAX_NAME+1];                         /* entry name */
  uint32_t nInodeDir;                            /* number of the directory inode */
  uint32_t nInodeEnt;                            /* number of the entry inode */
  uint32_t op;                                   /* operation to be performed */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Add a directory entry\n");
  if (batch == 0) printf("Inode number of the directory: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nInodeDir = (uint32_t) valInt;
  if (batch == 0) printf("Inode number of the entry: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nInodeEnt = (uint32_t) valInt;
  if (batch == 0) printf("Name of the entry: ");
  do
  { t = scanf ("%59s", name);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  if (batch == 0)
     { printf ("Operation to be performed:\n");
       printf ("    0 - add a generic entry to a directory\n");
       printf ("    1 - attach an entry to a directory to a directory\n");
       printf ("What is your option? ");
     }
  do
  { t = scanf ("%d", &valInt);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  op = (uint32_t) valInt;
  stat = soAddAttDirEntry (nInodeDir, name, nInodeEnt, op);
  if (stat != 0)
     printError (stat, "soAddAttDirEntry");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            fprintf(fl, "The entry whose name is %s was successfully added / attached to the parent directory.\n", name);
          }
}

/*
 * remove a directory entry
 */

static void removeDetachDirEntry (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  char name[MAX_NAME+1];                         /* entry name */
  uint32_t nInodeDir;                            /* number of the directory inode */
  uint32_t op;                                   /* operation to be performed */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Remove a directory entry\n");
  if (batch == 0) printf("Inode number of the directory: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nInodeDir = (uint32_t) valInt;
  if (batch == 0) printf("Name of the entry: ");
  do
  { t = scanf ("%59s", name);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  if (batch == 0)
     { printf ("Operation to be performed:\n");
       printf ("    0 - remove a generic entry from a directory\n");
       printf ("    1 - detach a generic entry from a directory\n");
       printf ("What is your option? ");
     }
  do
  { t = scanf ("%d", &valInt);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  op = (uint32_t) valInt;
  stat = soRemDetachDirEntry (nInodeDir, name, op);
  if (stat != 0)
     printError (stat, "soRemDetachDirEntry");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            fprintf(fl, "The entry whose name is %s was successfully removed / detached from the parent directory.\n",
            		name);
          }
}

/*
 * rename a directory entry
 */

static void renameDirEntry (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  char oldName[MAX_NAME+1];                      /* entry old name */
  char newName[MAX_NAME+1];                      /* entry new name */
  uint32_t nInodeDir;                            /* number of the directory inode */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Rename a directory entry\n");
  if (batch == 0) printf("Inode number of the directory: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nInodeDir = (uint32_t) valInt;
  if (batch == 0) printf("Present name of the entry: ");
  do
  { t = scanf ("%59s", oldName);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  if (batch == 0) printf("New name of the entry: ");
  do
  { t = scanf ("%59s", newName);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  stat = soRenameDirEntry (nInodeDir, oldName, newName);
  if (stat != 0)
     printError (stat, "soRenameDirEntry");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            fprintf(fl, "The entry name was successfully changed from %s to %s.\n", oldName, newName);
          }
}

/*
 * check if a directory is empty
 */

static void checkDirectoryEmptiness (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  uint32_t nInodeDir;                            /* number of the directory inode */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Check if a directory is empty\n");
  if (batch == 0) printf("Inode number of the directory: ");
  do
  { t = scanf ("%d", &valInt);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  nInodeDir = (uint32_t) valInt;
  stat = soCheckDirectoryEmptiness (nInodeDir);
  if ((stat != 0) && (stat != -ENOTEMPTY))
     printError (stat, "soCheckDirectoryEmptiness");
     else { if (fl == stdout) fprintf (fl, "\e[07;32m==>\e[0m ");
            if (stat == 0)
               fprintf(fl, "The directory whose inode is no. %u is empty.\n", nInodeDir);
               else fprintf(fl, "The directory whose inode is no. %u is not empty.\n", nInodeDir);
          }
}

/*
 * initialize symbolic link
 */

static void initSymLink (void)
{
  int t;                                         /* test flag */
  int valInt;                                    /* integer value */
  char path[MAX_PATH+1];                         /* path */
  SODataClust dc;                                /* data cluster whose contents is a string */
  uint32_t nInode;                               /* number of the softlin inode */
  SOInode inode;                                 /* inode buffer */
  int stat;                                      /* status of operation */

  if ((batch != 0) && (fl != stdout)) fprintf (fl, "Initialize a symbolic link\n");
  if (batch == 0) printf("Inode number of the softlink: ");
  do
  { t = scanf ("%d", &valInt);
  } while (t != 1);
  nInode = (uint32_t) valInt;
  if (batch == 0) printf("Contents of the symbolic link: ");
  do
  { t = scanf ("%255s", path);
    scanf ("%*[^\n]");
    scanf ("%*c");
  } while (t != 1);
  if ((stat = soReadInode (&inode, nInode, IUIN)) != 0)
     { printError (stat, "soInitSymLink");
       return;
     }
  if ((inode.mode & INODE_TYPE_MASK) != INODE_SYMLINK)
     { printError (-EINVAL, "soInitSymLink");
       return;
     }
  if ((stat = soReadFileCluster (nInode, 0, dc.info.de)) != 0)
     { printError (stat, "soInitSymLink");
       return;
     }
  strcpy ((char *) dc.info.de, path);
  stat = soWriteFileCluster (nInode, 0, dc.info.de);
  if (stat != 0)
     printError (stat, "soInitSymLink");
     else fprintf(fl, "The symbolic link was successfully initialized.\n");
}
#endif
