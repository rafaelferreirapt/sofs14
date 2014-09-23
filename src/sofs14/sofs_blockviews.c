/**
 *  \file sofs_blockviews.c (implementation file)
 *
 *  \brief Display the contents of a block/cluster from the storage device in different formats.
 *
 *  The formats are:
 *      \li block/cluster contents as hexadecimal data
 *      \li block/cluster contents as ascii/hexadecimal data
 *      \li block/cluster contents both as hexadecimal and ascii data
 *      \li block contents as superblock data
 *      \li block contents as inode data
 *      \li single inode data
 *      \li cluster contents as a byte stream
 *      \li cluster contents as a sub-array of data cluster references
 *      \li cluster contents as a sub-array of directory entry data.
 *
 *  \author Artur Pereira - October 2007
 *  \author Miguel Oliveira e Silva - September 2009
 *  \author António Rui Borges - August 2010 August 2011, September 2014
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include "sofs_const.h"
#include "sofs_superblock.h"
#include "sofs_inode.h"
#include "sofs_datacluster.h"
#include "sofs_direntry.h"

/** \brief Bit pattern description of the mode field in the inode data type */
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

/** \brief File system unmounting status strings */
static char *sPRU = "PRU";
static char *sNPRU = "NPRU";

/**
 *  \brief Display the block/cluster contents as hexadecimal data.
 *
 *  The contents is displayed in rows of 32 bytes each.
 *  Each row is labeled by the address of the first byte, also displayed in hexadecimal.
 *
 *  \param buf pointer to a buffer with block/cluster contents
 *  \param isCluster type of unit to display
 *         \li \c true, the unit is a cluster
 *         \li \c false, the unit is a block
 */

void printHex (void *buf, bool isCluster)
{
  unsigned char *byte;                           /* pointer to a byte array */
  int i;                                         /* counting variable */

  byte = (unsigned char *) buf;

  for (i = 0; i < (isCluster ? CLUSTER_SIZE : BLOCK_SIZE); i++)
  { /* print address label, if required */
    if ((i & 0x1f) == 0) printf ("%4.4x:", i);
    /* print byte */
    printf (" %.2x", byte[i]);
    /* terminate present line, if required */
    if ((i & 0x1f) == 0x1f) printf ("\n");
  }
}

/**
 *  \brief Display the block/cluster contents as ascii/hexadecimal data.
 *
 *  The contents is displayed in rows of 32 characters each.
 *  Each row is labeled by the address of the first byte, displayed in decimal.
 *
 *  Ascii is selected by default, hexadecimal is only used when the byte value, seen as a character, has no graphic
 *  representation.
 *
 *  \param buf pointer to a buffer with block/cluster contents
 *  \param isCluster type of unit to display
 *         \li \c true, the unit is a cluster
 *         \li \c false, the unit is a block
 */

void printAscii (void *buf, bool isCluster)
{
  unsigned char *c;                              /* pointer to a character array */
  char line[256];                                /* line to be printed */
  char *p_line = line;                           /* pointer to a character in the line */
  int i;                                         /* counting variable */

  c = (unsigned char *) buf;

  for (i = 0; i < (isCluster ? CLUSTER_SIZE : BLOCK_SIZE); i++)
  { /* print address label and initialize pointer, if required */
    if ((i & 0x1f) == 0)
       { printf ("%4.4d:", i);
         p_line = line;
       }
    /* add character to the line */
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
    }
    /* terminate and print present line, if required */
    if ((i & 0x1f) == 0x1f)
       { *p_line = '\0';
         printf("%s\n", line);
       }
  }
}

/**
 *  \brief Display the block/cluster contents both as hexadecimal and ascii data.
 *
 *  The contents is displayed in rows of 16 characters each.
 *  Each row is labeled by the address of the first byte, displayed in hexadecimal.
 *
 *  In each row, the hexadecimal representation is displayed first, followed by the ascii representation.
 *
 *  \param buf pointer to a buffer with block/cluster contents
 *  \param isCluster type of unit to display
 *         \li \c true, the unit is a cluster
 *         \li \c false, the unit is a block
 */

void printHexAscii (void *buf, bool isCluster)
{
  unsigned char *c;                              /* pointer to a character array */
  char line[256];                                /* line to be printed */
  char *p_line = line;                           /* pointer to a character in the line */
  int i;                                         /* counting variable */

  c = (unsigned char *) buf;

  for (i = 0; i < (isCluster ? CLUSTER_SIZE : BLOCK_SIZE); i++)
  { /* print address label and initialize pointer, if required */
    if ((i & 0x0f) == 0)
       { printf ("%4.4x: ", i);
         p_line = line;
       }
    /* print character and add it to the line */
    printf (" %.2x", c[i]);
    if ((i & 0x0f) == 0x0) p_line += sprintf (p_line, "    ");
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
    }
    /* terminate and print present line, if required */
    if ((i & 0x0f) == 0x0f)
       { *p_line = '\0';
         printf("%s\n", line);
       }
  }
}

/**
 *  \brief Display the block contents as superblock data.
 *
 *  The contents is displayed field by field, not literally, but in an operational way.
 *  Both decimal and ascii representations are used as required.
 *
 *  \param buf pointer to a buffer with block contents
 */

void printSuperBlock (void *buf)
{
  SOSuperBlock *p_sb;                            /* pointer to the superblock */
  int i;                                         /* counting variable */

  p_sb = (SOSuperBlock *) buf;

  /* header */

  printf ("Header\n");
  printf ("   Magic number = 0x%08"PRIX32"\n", p_sb->magic);
  printf ("   Version number = 0x%08"PRIX32"\n", p_sb->version);
  printf ("   Volume name: %-s\n", p_sb->name);
  printf ("   Total number of blocks in the device = %"PRIu32"\n", p_sb->nTotal);
  printf ("   Flag signaling if the file system was properly unmounted the last time it was mounted = ");
  if (p_sb->mStat == PRU)
     printf ("%s\n", sPRU);
     else printf ("%s\n", sNPRU);

  /* inode table metadata */

  printf ("Inode table metadata\n");
  printf ("   Physical number of the block where the table of inodes starts  = ");
  if (p_sb->iTableStart == NULL_BLOCK)
     printf ("(nil)\n");
     else printf ("%"PRIu32"\n", p_sb->iTableStart);
  printf ("   Number of blocks that the table of inodes comprises  = %"PRIu32"\n", p_sb->iTableSize);
  printf ("   Total number of inodes = %"PRIu32"\n", p_sb->iTotal);
  printf ("   Number of free inodes: %"PRIu32"\n", p_sb->iFree);
  printf ("   Index of the first free inode in the double-linked list (point of retrieval)  = ");
  if (p_sb->iHead == NULL_INODE)
     printf ("(nil)\n");
     else printf ("%"PRIu32"\n", p_sb->iHead);
  printf ("   Index of the last free inode in the double-linked list (point of insertion)  = ");
  if (p_sb->iTail == NULL_INODE)
     printf ("(nil)\n");
     else printf ("%"PRIu32"\n", p_sb->iTail);

  /* data zone */

  printf ("Data zone\n");
  printf ("   Physical number of the block where it starts (physical number of the first data cluster)  = ");
  if (p_sb->dZoneStart == NULL_BLOCK)
     printf ("(nil)\n");
     else printf ("%"PRIu32"\n", p_sb->dZoneStart);
  printf ("   Total number of data clusters = %"PRIu32"\n", p_sb->dZoneTotal);
  printf ("   Number of free data clusters = %"PRIu32"\n", p_sb->dZoneFree);
  printf ("   Retrieval cache of references to free data clusters\n");
  printf ("      Index of the first filled/free array element = %"PRIu32"\n", p_sb->dZoneRetriev.cacheIdx);
  printf ("      Reference cache contents:");
  if (p_sb->dZoneRetriev.cacheIdx == DZONE_CACHE_SIZE)
     printf (" empty");
     else for (i = p_sb->dZoneRetriev.cacheIdx; i < DZONE_CACHE_SIZE; i++)
          { if (p_sb->dZoneRetriev.cache[i] == NULL_CLUSTER)
               printf (" (nil)");
               else printf (" %"PRIu32"", p_sb->dZoneRetriev.cache[i]);
            if ((((i - p_sb->dZoneRetriev.cacheIdx + 1) % 10) == 0) && (i < (DZONE_CACHE_SIZE - 1)))
               { printf ("\n");
                 if (i < (DZONE_CACHE_SIZE - 1)) printf ("                     ");
               }
          }
  printf("\n");
  printf ("   Insertion cache of references to free data clusters\n");
  printf ("      Index of the first filled/free array element = %"PRIu32"\n", p_sb->dZoneInsert.cacheIdx);
  printf ("      Reference cache contents:");
  if (p_sb->dZoneInsert.cacheIdx == 0)
     printf (" empty");
     else for (i = 0; i < p_sb->dZoneInsert.cacheIdx; i++)
          { if (p_sb->dZoneInsert.cache[i] == NULL_BLOCK)
               printf (" (nil)");
               else printf (" %"PRIu32"", p_sb->dZoneInsert.cache[i]);
            if ((((i + 1) % 10) == 0) && (i < (p_sb->dZoneInsert.cacheIdx - 1)))
               { printf ("\n");
                 printf ("                     ");
               }
          }
  printf("\n");
  printf ("   Reference to the first data cluster in the double-linked list of free data\n");
  printf ("     clusters (point of retrieval)  = ");
  if (p_sb->dHead == NULL_CLUSTER)
     printf ("(nil)\n");
     else printf ("%"PRIu32"\n", p_sb->dHead);
  printf ("   Reference to the last data cluster in the double-linked list of free data\n");
  printf ("     clusters (point of insertion)  = ");
  if (p_sb->dTail == NULL_CLUSTER)
     printf ("(nil)\n");
     else printf ("%"PRIu32"\n", p_sb->dTail);
}

/**
 *  \brief Display the inode data.
 *
 *  The contents is displayed field by field, not literally, but in an operational way.
 *  Both decimal and ascii representations are used as required.
 *
 *  \param p_inode pointer to a buffer with inode contents
 *  \param nInode inode number
 */

void printInode (SOInode *p_inode, uint32_t nInode)
{
  char perm[10] = "rwxrwxrwx";                   /* access permissions array */
  int i;                                         /* counting variable */
  int m;                                         /* mask variable */
  time_t temp;                                   /* time indication in seconds */
  char timebuf[30];                              /* date and time string */

  /* print inode number */

  printf ("Inode #");
  if (nInode == NULL_INODE)
     printf ("(nil)\n");
     else printf ("%"PRIu32"\n", nInode);

  /* decouple and print mode field */

  printf ("type = %s, ", inodetypes[(int)((p_inode->mode & (INODE_FREE | INODE_TYPE_MASK)) >> 9)]);
  for (i = 8, m = 1; i >= 0; i--, m <<= 1)
    if ((p_inode->mode & m) == 0) perm[i] = '-';
  printf ("permissions = %s, ", perm);

  /* print reference count */

  printf ("refCount = %"PRIu16", ", p_inode->refCount);

  /* print owner and group IDs */

  printf ("owner = %"PRIu32", group = %"PRIu32"\n", p_inode->owner, p_inode->group);

  /* print file size in bytes and in clusters */

  printf ("size in bytes = %"PRIu32", size in clusters = %"PRIu32"\n", p_inode->size, p_inode->cluCount);

  /* decouple and print information about dates of file manipulation, if inode is in use, or about the inode references
   * within the double-linked list that holds the free inodes */

  if (p_inode->mode & INODE_FREE)
     /* inode is free */
     { printf ("prev = ");
       if (p_inode->vD2.prev == NULL_INODE)
           printf ("(nil), ");
           else printf ("%"PRIu32", ", p_inode->vD2.prev);
       printf ("next = ");
       if (p_inode->vD1.next == NULL_INODE)
           printf ("(nil)\n");
           else printf ("%"PRIu32"\n", p_inode->vD1.next);
     }
     else /* inode is in use */
          { temp = p_inode->vD1.aTime;
            ctime_r (&temp, timebuf);
            timebuf[strlen (timebuf) - 1] = '\0';
            printf ("atime = %s, ", timebuf);
            temp = p_inode->vD2.mTime;
            ctime_r (&temp, timebuf);
            timebuf[strlen (timebuf) - 1] = '\0';
            printf ("mtime = %s\n", timebuf);
          }

  /* print references to the data clusters that comprise the file information content */

  printf ("d[] = {");
  for (i = 0; i < N_DIRECT; i++)
  { if (i > 0) printf (" ");
    if (p_inode->d[i] == NULL_CLUSTER)
       printf ("(nil)");
       else printf ("%"PRIu32"", p_inode->d[i]);
  }
  printf ("}, ");
  printf ("i1 = ");
  if (p_inode->i1 == NULL_CLUSTER)
     printf ("(nil), ");
     else printf ("%"PRIu32", ", p_inode->i1);
  printf ("i2 = ");
  if (p_inode->i2 == NULL_CLUSTER)
     printf ("(nil)\n");
     else printf ("%"PRIu32"\n", p_inode->i2);
  printf ("----------------\n");
}

/**
 *  \brief Display the block contents as inode data.
 *
 *  The contents is displayed inode by inode and, within each inode, field by field, not literally, but in an
 *  operational way.
 *  Both decimal and ascii representations are used as required.
 *
 *  \param buf pointer to a buffer with block contents
 */

void printBlkInode (void *buf)
{
  SOInode *inode;                                /* pointer to an inode array */
  int i;                                         /* counting variable */

  inode = (SOInode *) buf;

  /* treat each inode stored in the block separately */

  for (i = 0; i < IPB; i++)
    printInode (&inode[i], i);
}

/**
 *  \brief Display the cluster content as a byte stream.
 *
 *  The header is displayed first in a single row.
 *  The body is displayed next as a byte stream in rows of 16 characters each.
 *  Each row is labeled by the address of the first byte, displayed in hexadecimal.
 *  Both decimal and ascii representations are used as required.
 *
 *  \param buf pointer to a buffer with the cluster contents
 */

void printCltByteStr (void *buf)
{
  SODataClust *clust;                            /* pointer to a cluster reference */

  clust = (SODataClust *) buf;

  printf ("prev = ");
  if (clust->prev == NULL_CLUSTER)
     printf ("(nil)");
     else printf (" %.10"PRIu32"", clust->prev);
  printf (" - next = ");
  if (clust->next == NULL_CLUSTER)
     printf ("(nil)");
     else printf (" %.10"PRIu32"", clust->next);
  printf (" - status = ");
  if (clust->stat == NULL_INODE)
     printf ("(nil)\n");
     else printf (" %.10"PRIu32"\n", clust->stat);

  unsigned char *c;                              /* pointer to a character array */
  char line[256];                                /* line to be printed */
  char *p_line = line;                           /* pointer to a character in the line */
  int i;                                         /* counting variable */

  c = (unsigned char *) clust->info.data;

  for (i = 0; i < BSLPC; i++)
  { /* print address label and initialize pointer, if required */
    if ((i & 0x0f) == 0)
       { printf ("%4.4x: ", i);
         p_line = line;
       }
    /* print character and add it to the line */
    printf (" %.2x", c[i]);
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
    }
    /* terminate and print present line, if required */
    if (((i & 0x0f) == 0x0f) || (i == (BSLPC - 1)))
       { *p_line = '\0';
         printf("%s\n", line);
       }
  }
}

/**
 *  \brief Display the cluster content as a sub-array of directory entries.
 *
 *  The header is displayed first in a single row.
 *  The body is displayed next a directory entry per row.
 *  Only the characters of the name field that have a graphical are represented by themselves, the remaining one are
 *  replaced by the <em>space</em> caracter.
 *  Both decimal and ascii representations are used as required.
 *
 *  \param buf pointer to a buffer with the cluster contents
 */

void printCltDirEnt (void *buf)
{
  SODataClust *clust;                            /* pointer to a cluster reference */
  int i, j;                                      /* counting variables */

  clust = (SODataClust *) buf;

  printf ("prev = ");
  if (clust->prev == NULL_CLUSTER)
     printf ("(nil)");
     else printf (" %.10"PRIu32"", clust->prev);
  printf (" - next = ");
  if (clust->next == NULL_CLUSTER)
     printf ("(nil)");
     else printf (" %.10"PRIu32"", clust->next);
  printf (" - status = ");
  if (clust->stat == NULL_INODE)
     printf ("(nil)\n");
     else printf (" %.10"PRIu32"\n", clust->stat);

  /* treat each directory entry stored in the cluster separately */

  for (i = 0; i < DPC; i++)
  { for (j = 0; j < MAX_NAME+1; j++)
      if ((clust->info.de[i].name[j] == '\0') || (clust->info.de[i].name[j] < ' ') || (clust->info.de[i].name[j] > 'z'))
         printf (" ");
         else printf ("%c", clust->info.de[i].name[j]);
    printf(": ");
    if (clust->info.de[i].nInode == NULL_INODE)
       printf ("(nil)\n");
       else printf ("%.10"PRIu32"\n", clust->info.de[i].nInode);
  }
}

/**
 *  \brief Display the cluster content as a sub-array of data cluster references.
 *
 *  The header is displayed first in a single row.
 *  The body is displayed next in rows of 8 references each.
 *  Each row is labeled by the address of the first reference.
 *  Both the address and the references are displayed in decimal.
 *
 *  \param buf pointer to a buffer with the cluster contents
 */

void printCltRef (void *buf)
{
  SODataClust *clust;                            /* pointer to a cluster reference */
  int i;                                         /* counting variable */

  clust = (SODataClust *) buf;

  printf ("prev = ");
  if (clust->prev == NULL_CLUSTER)
     printf ("(nil)");
     else printf (" %.10"PRIu32"", clust->prev);
  printf (" - next = ");
  if (clust->next == NULL_CLUSTER)
     printf ("(nil)");
     else printf (" %.10"PRIu32"", clust->next);
  printf (" - status = ");
  if (clust->stat == NULL_INODE)
     printf ("(nil)\n");
     else printf (" %.10"PRIu32"\n", clust->stat);

  for (i = 0; i < RPC; i++)
  { /* print address label, if required */
    if ((i & 0x07) == 0) printf ("%4.4d:", i);
    /* print reference to a cluster */
    if (clust->info.ref[i] == NULL_CLUSTER)
       printf ("   (nil)   ");
       else printf (" %.10"PRIu32"", clust->info.ref[i]);
    /* terminate present line, if required */
    if (((i & 0x07) == 0x07) || (i == (RPC - 1))) printf ("\n");
  }
}
