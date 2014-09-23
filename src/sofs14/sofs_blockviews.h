/**
 *  \file sofs_blockviews.h (interface file)
 *
 *  \brief Display the contents of a block/cluster from a storage device in different formats.
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

#ifndef SOFS_BLOCKVIEWS_H_
#define SOFS_BLOCKVIEWS_H_

#include <stdbool.h>

#include "sofs_inode.h"

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

extern void printHex (void *buf, bool isCluster);

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

extern void printAscii (void *buf, bool isCluster);

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

extern void printHexAscii (void *buf, bool isCluster);

/**
 *  \brief Display the block contents as superblock data.
 *
 *  The contents is displayed field by field, not literally, but in an operational way.
 *  Both decimal and ascii representations are used as required.
 *
 *  \param buf pointer to a buffer with block contents
 */

extern void printSuperBlock (void *buf);

/**
 *  \brief Display the block contents as inode data.
 *
 *  The contents is displayed inode by inode and, within each inode, field by field, not literally, but in an
 *  operational way.
 *  Both decimal and ascii representations are used as required.
 *
 *  \param buf pointer to a buffer with block contents
 */

extern void printBlkInode (void *buf);

/**
 *  \brief Display the inode data.
 *
 *  The contents is displayed field by field, not literally, but in an operational way.
 *  Both decimal and ascii representations are used as required.
 *
 *  \param p_inode pointer to a buffer with inode contents
 *  \param nInode inode number
 */

extern void printInode (SOInode *p_inode, uint32_t nInode);

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

extern void printCltByteStr (void *buf);

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

extern void printCltDirEnt (void *buf);

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

extern void printCltRef (void *buf);

#endif /* SOFS_BLOCKVIEWS_H_ */
