/**
 *  \file sofs_buffercacheinternals.h (interface file)
 *
 *  \brief Set of operations to internally manage the buffercache.
 *
 *  The buffercache is conceived as two double-linked lists: the first, based on the physical block number of the
 *  storage device it is referencing; the second, based on the order of last access to the block. Hence, one needs to
 *  define operations to insert, retrieve and access its nodes.
 *  One should notice that this module does not stand alone: it supposes a very tight coupling with the buffercache
 *  implementation, its only application.
 *
 *  The following operations are defined:
 *    \li access the first node of the double-linked list based on the physical block number of the storage device
 *    \li access the next node of the double-linked list based on the physical block number of the storage device
 *    \li check if a given block, whose physical number is given, has already been stored in the storage area
 *    \li insert a node in the two double-linked lists infrastructure
 *    \li retrieve a node from the two double-linked lists infrastructure
 *    \li move a node already present in the storage area to the head of the double-linked list based on the last access
 *        time.
 *
 *  \author António Rui Borges - July 2010 / August 2011
 */

#ifndef SOFS_BUFFERCACHEINTERNALS_H_
#define SOFS_BUFFERCACHEINTERNALS_H_

#include <stdint.h>

#include "sofs_buffercachenode.h"

/**
 *  \brief Access the first node of the double-linked list based on the physical block number of the storage device.
 *
 *  An iterator internal variable is set to the value of the argument and a pointer to the node pointed to by the
 *  iterator variable is returned.
 *
 *  \param head pointer to the head of the linked list based on the physical block number of the storage device
 *
 *  \return value of the <em>iterator</em> variable
 */

extern SOBufferCacheNode *getFirstNodeOnN (SOBufferCacheNode *head);

/**
 *  \brief Access the next node of the double-linked list based on the physical block number of the storage device.
 *
 *  The iterator internal variable is iterated if it does not already point to the last node of the linked list, and
 *  a pointer to the node pointed to by the iterator variable is returned.
 *
 *  \return value of the <em>iterator</em> variable
 */

extern SOBufferCacheNode *getNextNodeOnN (void);

/**
 *  \brief Check if a given block, whose physical number is given, has already been stored in the storage area.
 *
 *  The double-linked list based on the physical block number of the storage device is traversed from the node pointed
 *  to by the second argument to find out if there is a node whose contents belongs to the block whose physical number
 *  is passed as the first argument.
 *
 *  \param nBlock physical block number
 *  \param head pointer to the head of the linked list based on the block number of the storage device
 *
 *  \return pointer to the node where the block contents is stored, or \c NULL if the block has not been stored yet
 */

extern SOBufferCacheNode *searchNodeOnN (uint32_t nBlock, SOBufferCacheNode *head);

/**
 *  \brief Insert a node in the two double-linked lists infrastructure.
 *
 *  A node whose contents belongs to a block of the storage device, which is supposed not to be stored in the storage
 *  area yet, is inserted in the two double-linked lists infrastructure. If the node is already present or the storage
 *  area is inconsistent, nothing is done.
 *
 *  \param node pointer to the node to be inserted
 *  \param p_nLHead pointer to a location where the pointer to the head of the double linked list based on the physical
 *                  block number of the storage device, is stored
 *  \param p_lATLHead pointer to a location where the pointer to the head of the double-linked list based on the last
 *                    access time, is stored
 *  \param p_lATLTail pointer to a location where the pointer to the tail of the double-linked list based on the last
 *                    access time, is stored
 */

extern void insertNode (SOBufferCacheNode *node, SOBufferCacheNode **p_nLHead, SOBufferCacheNode **p_lATLHead,
                        SOBufferCacheNode **p_lATLTail);

/**
 *  \brief Retrieve a node from the two double-linked lists infrastructure.
 *
 *  The node which the tail of the double-linked list based on last access time points to, is retrieved from the two
 *  double-linked lists infrastructure. If the storage area is inconsistent, nothing is done.
 *
 *  \param p_nLHead pointer to a location where the pointer to the head of the double linked list based on the physical
 *                  block number of the storage device, is stored
 *  \param p_lATLHead pointer to a location where the pointer to the head of the double-linked list based on the last
 *                    access time, is stored
 *  \param p_lATLTail pointer to a location where the pointer to the tail of the double-linked list based on the last
 *                    access time, is stored
 *
 *  \return pointer to the retrieved node, or \c NULL if the storage area is empty or inconsistent
 */

extern SOBufferCacheNode *retrieveNode (SOBufferCacheNode **p_nLHead, SOBufferCacheNode **p_lATLHead,
                                        SOBufferCacheNode **p_lATLTail);

/**
 *  \brief Move the node to the head of the double-linked list based on last access time.
 *
 *  The node which is supposed to have been accessed, is retrieved from its location in the double-linked list based on
 *  the last access time and placed at the head of the list. If the node pointer is \c NULL or the storage area is
 *  inconsistent, nothing is done.
 *
 *  \param node pointer to the node to be inserted
 *  \param p_lATLHead pointer to a location where the pointer to the head of the double-linked list based on the last
 *                    access time, is stored
 *  \param p_lATLTail pointer to a location where the pointer to the tail of the double-linked list based on the last
 *                    access time, is stored
 */

extern void moveNodeAtHeadLAT (SOBufferCacheNode *node, SOBufferCacheNode **p_lATLHead, SOBufferCacheNode **p_lATLTail);

#endif /* SOFS_BUFFERCACHEINTERNALS_H_ */
