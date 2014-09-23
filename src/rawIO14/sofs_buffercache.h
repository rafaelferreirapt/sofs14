/**
 *  \file sofs_buffercache.h (interface file)
 *
 *  \brief Access to buffered/unbuffered raw disk blocks and clusters.
 *
 *  The mean transfer time of a data block (cluster) between main memory and disk is typically at least tens of
 *  thousands of times longer than the transfer time of an equal data block (cluster) between two different locations
 *  in main memory.
 *  Thus, the operating system tries to keep in a private storage area copies of the data blocks (clusters) whose
 *  probability of access in the near future is higher.
 *
 *  The buffercache may be regarded as a storage area resident in main memory having the ability to store K data blocks
 *  of the device's storage space.
 *  Data transfer between the main memory and the device works according to the following rules:
 *    \li every time a data block (cluster) is required for reading, it is looked up in the storage area: if it is
 *        there, the contents is copied to the supplied buffer location; otherwise, it is first read from the device
 *        and stored in the buffercache (a new node in the storage area is initialized to it and its status is marked
 *        \e same), then its contents is copied to the supplied buffer location, as before
 *    \li every time a data block (cluster) is required for writing, it is looked up in the storage area: if it is
 *        there, the contents of the supplied buffer is copied into it and its status is marked \e changed; otherwise,
 *        a new node in the storage area is initialized to this data block (cluster), the contents of the supplied
 *        buffer is copied into it and its status is marked <em>changed</em>, as before
 *    \li because the number of nodes in the storage area is finite, whenever it happens that no more free nodes are
 *        available, the node that has not been accessed for the longest time is selected for replacement: its contents,
 *        if needed (the status is marked <em>changed</em>), is first transfered to the device, then it becomes
 *        available for a new assignment.
 *
 *  The following operations are defined:
 *    \li initialize the storage area and assign it to the storage device
 *    \li unassign the storage area from the storage device and perform the required housekeeping duties
 *    \li read a block of data from the buffercache
 *    \li write a block of data to the buffercache
 *    \li flush a block of data to the storage device
 *    \li synchronize a block of data with the same block in the storage device
 *    \li read a cluster of data from the buffercache
 *    \li write a cluster of data to the buffercache
 *    \li flush a cluster of data to the storage device
 *    \li synchronize a cluster of data with the same cluster in the storage device.
 *
 *  \author Artur Carneiro Pereira - September 2007
 *  \author Miguel Oliveira e Silva - September 2009
 *  \author António Rui Borges - July 2010 / August 2011
 *
 *  \remarks In case an error occurs, all functions return a negative value which is the symmetric of the system error
 *           that better represents the error cause.
 *           (execute command <em>man errno</em> to get the list of system errors)
 */

#ifndef SOFS_BUFFERCACHE_H_
#define SOFS_BUFFERCACHE_H_

#include <stdint.h>
/** \brief the communication channel to the storage device is buffered */
#define BUF    0
/** \brief the communication channel to the storage device is unbuffered */
#define UNBUF  1

/**
 *  \brief Initialize the storage area and assign it to the storage device.
 *
 *  A communication channel is established with the storage device so that data transfers between main memory and the
 *  storage device may be minimized.
 *  This communication may be unbuffered or buffered: it will be unbuffered, if the second argument is \c UNBUF , and
 *  buffered, in any other case.
 *
 *  \param devname absolute path to the Linux file that simulates the storage device
 *  \param type type of the communication channel that is opened
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the argument is \c NULL
 *  \return -\c EBUSY, if the storage area is already in use or the device is already opened
 *  \return -\c ELIBBAD, if the supporting file size is invalid
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soOpenBufferCache (const char *devname, uint32_t type);

/**
 *  \brief Unassign the storage area from the storage device and perform the required housekeeping duties.
 *
 *  The buffered/unbuffered communication channel previously established with the storage device is closed.
 *  This means, namely, that the contents of the storage area is flushed into the storage device to keep data
 *  consistent.
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -\c ELIBBAD, if the internal data is inconsistent
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soCloseBufferCache (void);

/**
 *  \brief Read a block of data from the buffercache.
 *
 *  Both the physical number of the data block to be read and a pointer to a previously allocated buffer are supplied
 *  as arguments.
 *
 *  \param n physical number of the data block to be read from
 *  \param buf pointer to the buffer where the data must be read into
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>buffer pointer</em> is \c NULL or the <em>block number</em> is out of range
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on reading or writing
 *  \return -\c ELIBBAD, if the buffercache is inconsistent
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soReadCacheBlock (uint32_t n, void *buf);

/**
 *  \brief Write a block of data to the buffercache.
 *
 *  Both the physical number of the data block to be written and a pointer to a previously allocated buffer are supplied
 *  as arguments.
 *
 *  \param n physical number of the block to be written into
 *  \param buf pointer to the buffer containing the data to be written from
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if <em>buffer pointer</em> is \c NULL or <em>block number</em> is out of range
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -\c ELIBBAD, if the buffercache is inconsistent
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soWriteCacheBlock (uint32_t n, void *buf);

/**
 *  \brief Flush a block of data to the storage device.
 *
 *  Both the physical number of the data block to be written and a pointer to a previously allocated buffer are supplied
 *  as arguments.
 *
 *  \param n physical number of the block to be flushed
 *  \param buf pointer to the buffer containing the data to be written from
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if <em>buffer pointer</em> is \c NULL or <em>block number</em> is out of range
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -\c ELIBBAD, if the buffercache is inconsistent
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soFlushCacheBlock (uint32_t n, void *buf);

/**
 *  \brief Synchronize a block of data with the same block in the storage device.
 *
 *  The physical number of the data block to be synchronized is supplied as argument.
 *
 *  \param n physical number of the block to be synchronized
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if <em>block number</em> is out of range
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -\c ELIBBAD, if the buffercache is inconsistent
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soSyncCacheBlock (uint32_t n);

/**
 *  \brief Read a cluster of data from the buffercache.
 *
 *  The device is organized as a linear array of data blocks. A cluster is a group of successive blocks.
 *  Both the physical number of the first block of the data cluster to be read and a pointer to a previously allocated
 *  buffer are supplied as arguments.
 *
 *  \param n physical number of the first block of the data cluster to be read from
 *  \param buf pointer to the buffer where the data must be read into
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>buffer pointer</em> is \c NULL or the <em>block number</em> is out of range
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on reading or writing
 *  \return -\c ELIBBAD, if the buffercache is inconsistent
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soReadCacheCluster (uint32_t n, void *buf);

/**
 *  \brief Write a cluster of data to the buffercache.
 *
 *  The device is organized as a linear array of data blocks. A cluster is a group of successive blocks.
 *  Both the physical number of the first block of the data cluster to be written and a pointer to a previously
 *  allocated buffer are supplied as arguments.
 *
 *  \param n physical number of the first block of the data cluster to be written into
 *  \param buf pointer to the buffer containing the data to be written from
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if <em>buffer pointer</em> is \c NULL or <em>block number</em> is out of range
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -\c ELIBBAD, if the buffercache is inconsistent
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soWriteCacheCluster (uint32_t n, void *buf);

/**
 *  \brief Flush a cluster of data to the storage device.
 *
 *  The device is organized as a linear array of data blocks. A cluster is a group of successive blocks.
 *  Both the physical number of the first block of the data cluster to be flushed and a pointer to a previously
 *  allocated buffer are supplied as arguments.
 *
 *  \param n physical number of the first block of the data cluster to be flushed
 *  \param buf pointer to the buffer containing the data to be written from
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if <em>buffer pointer</em> is \c NULL or <em>block number</em> is out of range
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -\c ELIBBAD, if the buffercache is inconsistent
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soFlushCacheCluster (uint32_t n, void *buf);

/**
 *  \brief Synchronize a cluster of data with the same cluster in the storage device.
 *
 *  The device is organized as a linear array of data blocks. A cluster is a group of successive blocks.
 *  The physical number of the data cluster to be synchronized is supplied as argument.
 *
 *  \param n physical number of the first block of the data cluster to be synchronized
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if <em>block number</em> is out of range
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -\c ELIBBAD, if the buffercache is inconsistent
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soSyncCacheCluster (uint32_t n);

#endif /* SOFS_BUFFERCACHE_H_ */
