/**
 *  \file sofs_datacluster.h (interface file)
 *
 *  \brief Definition of the data cluster data type.
 *
 *  It specifies the file system metadata which describes the data cluster content.
 *
 *  \author António Rui Borges - August 2011, September 2014
 */

#ifndef SOFS_DATACLUSTER_H_
#define SOFS_DATACLUSTER_H_

#include <stdint.h>

#include "sofs_const.h"
#include "sofs_direntry.h"

/** \brief reference to a null data cluster */
#define NULL_CLUSTER ((uint32_t)(~0U))

/** \brief size of the byte stream per data cluster */
#define BSLPC (CLUSTER_SIZE - (3 * sizeof (uint32_t)))

/** \brief number of data cluster references per data cluster */
#define RPC ((CLUSTER_SIZE / sizeof (uint32_t)) - 3)

/** \brief number of directory entries per data cluster */
#define DPC ((CLUSTER_SIZE - (3 * sizeof (uint32_t))) / sizeof (SODirEntry))

/**
 *  \brief Definition of a data cluster information content data type.
 *
 *  It describes the different interpretations for the information content of a data cluster in use.
 *
 *  It may either contain:
 *     \li a stream of bytes
 *     \li a sub-array of data cluster references
 *     \li a sub-array of directory entries.
 */

 union infoContent
    {
      /** \brief byte stream */
       unsigned char data[BSLPC];
      /** \brief sub-array of data cluster references */
       uint32_t ref[RPC];
      /** \brief sub-array of directory entries */
       SODirEntry de[DPC];
    };

/**
 *  \brief Definition of the data cluster data type.
 *
 *  It is divided in:
 *     \li <em>header</em> - metadata concerning the treatment of a data cluster as a node which may belong to the
 *         double-linked list that forms the general repository of free data clusters or the file information content
 *         (references to the previous and the next node) and its status in all cases
 *     \li <em>body</em> - information content of the data cluster.
 */

typedef struct soDataClust
{
  /* Header */

   /** \brief if the data cluster is free and resides in the general repository of free data clusters or is in use and
    *         belongs to the information content of a file, reference to the previous data cluster in the double-linked
    *         list; when it is free and its reference is in one of the caches in the superblock, reference to
    *         NULL_CLUSTER */
    uint32_t prev;
   /** \brief if the data cluster is free and resides in the general repository of free data clusters or is in use and
    *         belongs to the information content of a file, reference to the next data cluster in the double-linked
    *         list; when it is free and its reference is in one of the caches in the superblock, reference to
    *         NULL_CLUSTER */
    uint32_t next;
   /** \brief status of the data cluster
    *     \li <em>clean</em> - reference to NULL_INODE if it has not been used before or if the file it has once
    *                          belonged to has been deleted permanently
    *     \li <em>dirty</em> - reference to a file inode number if it is in use or if the file it has once belonged to,
    *                          although deleted, may still be recovered
    */
    uint32_t stat;

  /* Body */

   /** \brief cluster information content */
    union infoContent info;
} SODataClust;

#endif /* SOFS_DATACLUSTER_H_ */
