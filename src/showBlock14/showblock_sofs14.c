/**
 *  \file showblock_sofs14.c (implementation file)
 *
 *  \brief The SOFS14 block/cluster display tool.
 *
 *  It displays preselected blocks/clusters of the storage device, supposed to contain file system data or metadata.
 *  With it, the storage device is envisaged as an array of blocks, some of them joined in clusters.
 *
 *  The following display formats are available:
 *      \li block/cluster contents as hexadecimal data
 *      \li block/cluster contents as ascii/hexadecimal data
 *      \li block/cluster contents both as hexadecimal and ascii data
 *      \li block contents as superblock data
 *      \li block contents as a sub-array of inode entries
 *      \li cluster contents as a byte stream
 *      \li cluster contents as a sub-array of directory entries
 *      \li cluster contents as a sub-array of data cluster references.
 *
 *  SINOPSIS:
 *  <P><PRE>                   showblock_sofs14 OPTIONS supp-file
 *
 *                OPTIONS:
 *                 -x blockNumber   --- show the block contents as hexadecimal data
 *                 -X clusterNumber --- show the cluster contents as hexadecimal data
 *                 -b blockNumber   --- show the block contents as ascii/hexadecimal data
 *                 -B clusterNumber --- show the cluster contents as ascii/hexadecimal data
 *                 -a blockNumber   --- show the block contents both as hexadecimal and ascii data
 *                 -A clusterNumber --- show the cluster contents both as hexadecimal and ascii data
 *                 -s blockNumber   --- show the block contents as superblock data
 *                 -i blockNumber   --- show the block contents as a sub-array of inode entries
 *                 -T clusterNumber --- show the cluster contents as a byte stream
 *                 -D clusterNumber --- show the cluster contents as a sub-array of directory entries
 *                 -R clusterNumber --- show the cluster contents as a sub-array of data cluster references
 *                 -h               --- print this help.</PRE>
 *
 *  \remarks All cluster and block numbers in OPTIONS are physical numbers (indexes of the array of blocks that
 *           represent the storage device at this abstraction level). Furthermore, a cluster number is the number of
 *           the first block that comprises it.

 *  \author João Pagaime Silva - January 2006
 *  \author Artur Carneiro Pereira - September 2006
 *  \author Miguel Oliveira e Silva - September 2009
 *  \author António Rui Borges - August 2010 - August 2011, September 2014
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#include "sofs_const.h"
#include "sofs_rawdisk.h"
#include "sofs_blockviews.h"

/* Allusion to internal functions */

static void printUsage (char *cmd_name);
static void printError (int errcode, char *cmd_name);

/* The main function */

int main (int argc, char *argv[])
{
  void (*print1) (void *buf) = NULL;                       /* display function prototype with one parameter */
  void (*print2) (void *buf, bool isCluster) = NULL;       /* display function prototype with two parameters */
  char *msg = NULL;                                        /* type of display */
  bool isCluster;                                          /* type of unit to display */
  int unitNumber = 0;                                      /* unit number (default, zero) */

  /* process command line options */

  int opt;                                       /* selected option */

  if ((opt = getopt (argc, argv, "x:X:a:A:b:B:s:i:T:D:R:h")) == -1)
     { fprintf (stderr, "%s: An option is needed.\n", basename (argv[0]));
       printUsage (basename (argv[0]));
       return EXIT_FAILURE;
     }
  if (opt == '?')
     { fprintf (stderr, "%s: Wrong option.\n", basename (argv[0]));
       printUsage (basename (argv[0]));
       return EXIT_FAILURE;
     }
  if (opt == 'h')
     { /* help mode */
       printUsage (basename (argv[0]));
       return EXIT_SUCCESS;
     }
  unitNumber = (int32_t) atoi (optarg);
  if (unitNumber < 0)
     { fprintf (stderr, "%s: Negative unit number.\n", basename (argv[0]));
       printUsage (basename (argv[0]));
       return EXIT_FAILURE;
     }
  if (getopt (argc, argv, "x:X:a:A:b:B:s:i:T:D:R:h") != -1)
     { fprintf (stderr, "%s: Too many options.\n", basename (argv[0]));
       printUsage (basename (argv[0]));
       return EXIT_FAILURE;
     }
  switch (opt)
  { case 'x': /* show the block contents as hexadecimal data */
              isCluster = false;
              print2 = printHex;
              msg = "as hexadecimal";
              break;
    case 'X': /* show the cluster contents as hexadecimal data */
              isCluster = true;
              print2 = printHex;
              msg = "as hexadecimal";
              break;
    case 'a': /* show the block contents both as hexadecimal and ascii data */
              isCluster = false;
              print2 = printHexAscii;
              msg = "as hexadecimal+ascii";
              break;
    case 'A': /* show the cluster contents both as hexadecimal and ascii data */
              isCluster = true;
              print2 = printHexAscii;
              msg = "as hexadecimal+ascii";
              break;
    case 'b': /* show the block contents as ascii/hexadecimal data */
              isCluster = false;
              print2 = printAscii;
              msg = "as ascii";
              break;
    case 'B': /* show the cluster contents as ascii/hexadecimal data */
              isCluster = true;
              print2 = printAscii;
              msg = "as ascii";
              break;
    case 's': /* show block contents as superblock data */
              isCluster = false;
              print1 = printSuperBlock;
              msg = "as superblock data";
              break;
    case 'i': /* show the block contents as a sub-array of inode entries */
              isCluster = false;
              print1 = printBlkInode;
              msg = "as a sub-array of inode entries";
              break;
    case 'T': /* show the cluster contents as a byte stream */
              isCluster = true;
              print1 = printCltByteStr;
              msg = "as a byte stream";
              break;
    case 'D': /* show the cluster contents as a sub-array of directory entries */
              isCluster = true;
              print1 = printCltDirEnt;
              msg = "as a sub-array of directory entries";
              break;
    case 'R': /* show the cluster contents as a sub-array of data cluster references */
              isCluster = true;
              print1 = printCltRef;
              msg = "as a sub-array of data cluster references";
              break;
    default:  fprintf (stderr, "%s: It should not have happened.\n", basename (argv[0]));
              printUsage (basename (argv[0]));
              return EXIT_FAILURE;
  }
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

  /* display of the storage device blocks/clusters is going to start */

  unsigned char buffer[CLUSTER_SIZE];            /* buffer to store block/cluster contents */
  int status;                                    /* status of operation */

  /* open a direct communication channel with the storage device */

  uint32_t dummy;                                /* dummy variable */

  if ((status = soOpenDevice (argv[optind], &dummy)) != 0)
     { printError (status, basename (argv[0]));
       return EXIT_FAILURE;
     }

  /* read block/cluster */

  if (isCluster)
     status = soReadRawCluster (unitNumber, buffer);
     else status = soReadRawBlock (unitNumber, buffer);
  if (status == -EINVAL)
     { fprintf (stderr, "%s: Unit number too large.\n", basename (argv[0]));
       return EXIT_FAILURE;
     }
     else if (status != 0)
             { printError (status, basename (argv[0]));
               return EXIT_FAILURE;
             }

  /* display block/cluster */

  if ((opt == 's') || (opt == 'i') || (opt == 'T') || (opt == 'D') || (opt == 'R'))
     { if ((opt == 'T') || (opt == 'D') || (opt == 'R'))
          printf ("Cluster ");
          else printf ("Block ");
       printf ("%"PRIu32" %s\n", unitNumber, msg);
       print1 (buffer);
     }
     else { if (isCluster)
               printf ("Cluster ");
               else printf ("Block ");
            printf ("%"PRIu32" %s\n", unitNumber, msg);
            print2 (buffer, isCluster);
          }

  /* close the communication channel with the storage device */

  if ((status = soCloseDevice ()) != 0)
     { printError (status, basename (argv[0]));
       return EXIT_FAILURE;
     }

  /* that's all */

  return EXIT_SUCCESS;

} /* end of main */

/*
 * print help message
 */

static void printUsage (char *cmd_name)
{
  printf ("Sinopsis: %s OPTIONS supp-file\n"
          "  OPTIONS:\n"
          "  -x blockNumber   --- show the block contents as hexadecimal data\n"
          "  -X clusterNumber --- show the cluster contents as hexadecimal data\n"
          "  -b blockNumber   --- show the block contents as ascii/hexadecimal data\n"
          "  -B clusterNumber --- show the cluster contents as ascii/hexadecimal data\n"
          "  -a blockNumber   --- show the block contents both as hexadecimal and ascii data\n"
          "  -A clusterNumber --- show the cluster contents both as hexadecimal and ascii data\n"
          "  -s blockNumber   --- show the block contents as superblock data\n"
          "  -i blockNumber   --- show the block contents as a sub-array of inode entries\n"
          "  -T clusterNumber --- show the cluster contents as a byte stream\n"
          "  -D clusterNumber --- show the cluster contents as a sub-array of directory entries\n"
          "  -R clusterNumber --- show the cluster contents as a sub-array of data cluster references\n"
          "  -h               --- print this help\n", cmd_name);
}

/*
 * print error message
 */

static void printError (int errcode, char *cmd_name)
{
  fprintf(stderr, "%s: error #%d - %s.\n", cmd_name, -errcode, strerror (-errcode));
}
