/**
 *  \file sofs_inode.h (interface file)
 *
 *  \brief Definition of the inode data type.
 *
 *  It specifies the file system metadata which describes how files (whether a regular file, a directory or a symbolic
 *  link) are identified internally. Therefore, the inode number acts as the operational file identifier.
 *
 *  \author Artur Carneiro Pereira - September 2008
 *  \author Miguel Oliveira e Silva - September 2009
 *  \author António Rui Borges - September 2010 - August 2011, September 2014
 */

#ifndef SOFS_INODE_H_
#define SOFS_INODE_H_

#include <stdint.h>

#include "sofs_const.h"
#include "sofs_datacluster.h"

/** \brief number of inodes per block */
#define IPB (BLOCK_SIZE / sizeof(SOInode))

/** \brief reference to a null inode */
#define NULL_INODE ((uint32_t)(~0U))

/** \brief flag signaling inode is free */
#define INODE_FREE (1<<12)

/** \brief flag signaling inode describes a directory */
#define INODE_DIR (1<<11)

/** \brief flag signaling inode describes a regular file */
#define INODE_FILE (1<<10)

/** \brief flag signaling inode describes a symlink */
#define INODE_SYMLINK (1<<9)

/** \brief inode type mask */
#define INODE_TYPE_MASK (INODE_DIR | INODE_FILE | INODE_SYMLINK)

/** \brief flag signaling owner - read permission */
#define INODE_RD_USR (0400)

/** \brief flag signaling owner - write permission */
#define INODE_WR_USR (0200)

/** \brief flag signaling owner - execution permission */
#define INODE_EX_USR (0100)

/** \brief flag signaling group - read permission */
#define INODE_RD_GRP (0040)

/** \brief flag signaling group - write permission */
#define INODE_WR_GRP (0020)

/** \brief flag signaling group - execution permission */
#define INODE_EX_GRP (0010)

/** \brief flag signaling other - read permission */
#define INODE_RD_OTH (0004)

/** \brief flag signaling other - write permission */
#define INODE_WR_OTH (0002)

/** \brief flag signaling other - execution permission */
#define INODE_EX_OTH (0001)

/** \brief direct block references in the inode */
#define N_DIRECT (7)

/** \brief maximum size of a file information content in number of clusters */
#define MAX_FILE_CLUSTERS (N_DIRECT + RPC + (RPC * RPC))

/** \brief maximum size of a file information content in bytes */
#define MAX_FILE_SIZE (BSLPC * MAX_FILE_CLUSTERS)

/** \brief maximum size of a file in cluster count */
#define MAX_CLUSTER_COUNT (MAX_FILE_CLUSTERS + 2 + RPC)

/** \brief Different interpretations for the variable context of the inode depending on the inode status (in use/free):
 *         type 1 context.
 *
 *  It may either contain:
 *     \li the time of last file access when the inode is in use
 *     \li the reference to the next inode when the inode is free.
 */

    union inodeFirst
    {
     /** \brief if the inode is in use, time of last file access */
      uint32_t aTime;
     /** \brief if the inode is free, reference to the next inode in the double-linked list of free inodes */
      uint32_t next;
    };

    /** \brief Different interpretations for the variable context of the inode depending on the inode status
     *         (in use/free): type 2 context.
     *
     *  It may either contain:
     *     \li the time of modification file access when the inode is in use
     *     \li the reference to the previous inode when the inode is free.
     */

        union inodeSecond
        {
         /** \brief if the inode is in use, time of last file modification */
          uint32_t mTime;
         /** \brief if the inode is free, reference to the previous inode in the double-linked list of free inodes */
          uint32_t prev;
        };

/**
 *  \brief Definition of the inode data type.
 *
 *  It is divided in:
 *     \li <em>header</em> - metadata concerning the file type, access rights, ownership and size (both in byte count
 *                           and in the number of data clusters which are allocated to it)
 *     \li <em>variable context</em> - metadata dependent on the inode status: if it is in use, the times of last
 *                                     access/modification of the file; if it is free, references to the previous/next
 *                                     inode in the double-linked list of free inodes
 *     \li <em>information content</em> - list of references to data clusters where the file information content is
 *                                        located.
 */

typedef struct soInode
{
   /** \brief inode mode: it stores the file type (either a regular file, a directory or a symbolic link) and its
    *         access permissions
    *     \li bits 2-0 rwx permissions for other
    *     \li bits 5-3 rwx permissions for group
    *     \li bits 8-6 rwx permissions for owner
    *     \li bit 9 is set if it represents a symbolic link
    *     \li bit 10 is set if it represents a regular file
    *     \li bit 11 is set if it represents a directory
    *     \li bit 12 is set if it is free
    *     \li the other bits are presently reserved
    */
    uint16_t mode;
   /** \brief reference count: number of hard links (directory entries) associated to the inode */
    uint16_t refCount;
   /** \brief user ID of the file owner */
    uint32_t owner;
   /** \brief group ID of the file owner */
    uint32_t group;
   /** \brief file size in bytes: the farthest position from the beginning of the file information content + 1 where a
    *         byte has been written */
    uint32_t size;
   /** \brief cluster count: total number of data clusters attached to the file (this means both the data clusters that
    *         hold the file information content and the ones that hold the auxiliary data structures for indirect
    *         referencing) */
    uint32_t cluCount;

   /** \brief variable context of type 1 depending on the inode status: in use/free */
    union inodeFirst vD1;
   /** \brief variable context of type 2 depending on the inode status: in use/free */
    union inodeSecond vD2;

   /** \brief direct references to the data clusters that comprise the file information content */
    uint32_t d[N_DIRECT];
   /** \brief reference to the data cluster that holds the next group of direct references to the data clusters that
    *         comprise the file information content */
    uint32_t i1;
   /** \brief reference to the data cluster that holds an array of indirect references holding in its turn successive
    *         groups of direct references to the data clusters that comprise the file information content */
    uint32_t i2;
} SOInode;

#endif /* SOFS_INODE_H_ */
