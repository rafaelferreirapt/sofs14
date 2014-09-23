/**
 *  \file sofs_superblock.h (interface file)
 *
 *  \brief Definition of the superblock data type.
 *
 *  It specifies the file system metadata which describes its internal architecture.
 *
 *  \author Artur Carneiro Pereira - September 2008
 *  \author Miguel Oliveira e Silva - September 2009
 *  \author António Rui Borges - September 2010 / September 2014
 */

#ifndef SOFS_SUPERBLOCK_H_
#define SOFS_SUPERBLOCK_H_

#include <stdint.h>

#include "sofs_const.h"

/** \brief sofs14 magic number */
#define MAGIC_NUMBER         (0x65FE)

/** \brief sofs14 version number */
#define VERSION_NUMBER       (0x2014)

/** \brief maximum length of volume name */
#define PARTITION_NAME_SIZE  (23)

/** \brief constant signaling the file system was properly unmounted the last time it was mounted */
#define PRU                  0

/** \brief constant signaling the file system was not properly unmounted the last time it was mounted */
#define NPRU                 1

/** \brief reference to a null data block */
#define NULL_BLOCK           ((uint32_t)(~0U))

/** \brief size of cache */
#define DZONE_CACHE_SIZE     (50)

/** \brief size of reserved area */
#define RESERV_AREA_SIZE     (BLOCK_SIZE - PARTITION_NAME_SIZE + 1 - 15 * sizeof(uint32_t) - 2 * sizeof(struct fCNode))

/**
 *  \brief Definition of the reference cache data type.
 *
 *  It describes an easy access temporary storage area within the superblock for references to free data clusters.
 */

struct fCNode
    {
       /** \brief index of the first filled/free array element */
        uint32_t cacheIdx;
       /** \brief storage area whose elements are the logical numbers of free data clusters */
        uint32_t cache[DZONE_CACHE_SIZE];
    };

/**
 *  \brief Definition of the superblock data type.
 *
 *  It contains global information about the file system layout, namely the size and the location of the remaining
 *  parts.
 *
 *  It is divided in:
 *     \li <em>header</em> - that contains data about the type, the version, the name, the size in number of physical
 *         blocks and the consistency status
 *     \li <em>inode table metadata</em> - concerning its location, size in number of blocks, total number of inodes
 *         and number of free inodes; the inode table is primarily designed as an array of inodes; however, the free
 *         inodes form a double-linked list whose head and tail location are also provided (it may be seen as a dynamic
 *         FIFO that links together, using the inodes themselves as nodes, all the free inodes)
 *     \li <em>data zone metadata</em> - concerning its location, size in total number of data clusters, number of free
 *         data clusters and its three main reference components: the insertion and retrieval caches that provide an
 *         easy access temporary storage to references (static structures resident within the superblock itself) and
 *         the location of the head and the tail of the general repository of free data clusters double-linked list
 *         (dynamic FIFO that links together, using the data  clusters themselves as nodes, all the free data clusters
 *         whose references are not in the above mentioned caches).
 */

typedef struct soSuperBlock
{
  /* Header */

   /** \brief magic number - file system identification number (should be MAGIC_NUMBER macro value) */
    uint32_t magic;
   /** \brief version number (should be VERSION_NUMBER macro value) */
    uint32_t version;
   /** \brief volume name */
    unsigned char name[PARTITION_NAME_SIZE+1];
   /** \brief total number of blocks in the device */
    uint32_t nTotal;
   /** \brief flag signaling if the file system was properly unmounted the last time it was mounted
    *     \li PRU - if properly unmounted
    *     \li NPRU - if not properly unmounted
    */
    uint32_t mStat;

  /* Inode table metadata */

   /** \brief physical number of the block where the table of inodes starts */
    uint32_t iTableStart;
   /** \brief number of blocks that the table of inodes comprises */
    uint32_t iTableSize;
   /** \brief total number of inodes */
    uint32_t iTotal;
   /** \brief number of free inodes */
    uint32_t iFree;
   /** \brief index of the array element that forms the head of the double-linked list of free inodes (point of
    *         retrieval) */
    uint32_t iHead;
   /** \brief index of the array element that forms the tail of the double-linked list of free inodes (point of
    *         insertion) */
    uint32_t iTail;

  /* Data zone metadata */

   /** \brief physical number of the block where the data zone starts (physical number of the first data cluster) */
    uint32_t dZoneStart;
   /** \brief total number of data clusters */
    uint32_t dZoneTotal;
   /** \brief number of free data clusters */
    uint32_t dZoneFree;
   /** \brief retrieval cache of references (or logical numbers) to free data clusters */
    struct fCNode dZoneRetriev;
   /** \brief insertion cache of references (or logical numbers) to free data clusters */
    struct fCNode dZoneInsert;
   /** \brief logical number of the data cluster that forms the head of the double-linked list of free data clusters
    *         (point of retrieval) */
    uint32_t dHead;
   /** \brief logical number of the data cluster that forms the tail of the double-linked list of free data clusters
    *         (point of insertion) */
    uint32_t dTail;

  /* Padded area to ensure superblock structure is BLOCK_SIZE bytes long */

   /** \brief reserved area */
    unsigned char reserved[RESERV_AREA_SIZE];
} SOSuperBlock;

#endif /* SOFS_SUPERBLOCK_H_ */
