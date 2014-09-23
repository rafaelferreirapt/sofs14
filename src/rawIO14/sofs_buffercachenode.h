/**
 *  \file sofs_buffercachenode.h (interface file)
 *
 *  \brief Definition of the buffercache node data type.
 *
 *  \author António Rui Borges - July 2010
 */

#ifndef SOFS_BUFFERCACHENODE_H_
#define SOFS_BUFFERCACHENODE_H_

#include <stdint.h>

#include "sofs_const.h"

/**
 *  \brief Definition of the buffercache node data type.
 *
 *  The buffercache is conceived as two double-linked lists: the first, based on the block number of the storage device
 *  it is referencing; the second, based on the order of last access to the block.
 *  So, besides the pointers which are required to implement this dynamic structure, each node contains:
 *    \li a buffer area to store locally the contents of the referenced block
 *    \li the physical block number
 *    \li a status flag which signals whether the block contents is, or is not, synchronized with the contents of the
 *        corresponding block in the storage device.
 */

typedef struct soBufferCacheNode
{
   /** \brief contents of the data block */
    unsigned char buffer[BLOCK_SIZE];
   /** \brief physical block number */
    uint32_t n;
   /** \brief status of the data block
    *  \li <em>same</em> - the contents is the same as the corresponding block in the storage device
    *  \li <em>changed</em> - the contents is potentially different
    */
    uint32_t stat;

   /** \brief double-linked list based on block number:
    *         pointer to previous node */
    struct soBufferCacheNode *n_prev;
   /** \brief double-linked list based on block number:
    *         pointer to next node */
    struct soBufferCacheNode *n_next;

   /** \brief double-linked list based on last access time:
    *         pointer to previous node */
    struct soBufferCacheNode *access_prev;
   /** \brief double-linked list based on last access time:
    *         pointer to next node */
    struct soBufferCacheNode *access_next;
} SOBufferCacheNode;

/** \brief the contents of a block in the storage area is the same as the corresponding block in the storage device */
#define SAME    0
/** \brief the contents of a block in the storage area is potentially different from the corresponding block in the
 *         storage device */
#define CHANGED 1

#endif /* SOFS_BUFFERCACHENODE_H_ */
