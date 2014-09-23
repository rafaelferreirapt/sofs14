/**
 *  \file sofs_rawdisk.h (interface file)
 *
 *  \brief Access to raw disk blocks and clusters.
 *
 *  The storage device is presently a Linux file which simulates a magnetic disk.
 *  The following operations are defined:
 *    \li open a communication channel with the storage device
 *    \li close the communication channel previously established
 *    \li read a block of data from the storage device
 *    \li write a block of data to the storage device
 *    \li read a cluster of data from the storage device
 *    \li write a cluster of data to the storage device.
 *
 *  \author Artur Carneiro Pereira - September 2007
 *  \author Miguel Oliveira e Silva - September 2009
 *  \author António Rui Borges - July 2010
 *
 *  \remarks In case an error occurs, all functions return a negative value which is the symmetric of the system error
 *           that better represents the error cause.
 *           (execute command <em>man errno</em> to get the list of system errors)
 */

#ifndef SOFS_RAWDISK_H_
#define SOFS_RAWDISK_H_

#include <stdint.h>

/**
 *  \brief Open the storage device.
 *
 *  A communication channel is established with the storage device.
 *  It is supposed that no communication channel was previously established.
 *  The Linux file that simulates the storage device must exist and have a size multiple of the block size.
 *
 *  \param devname absolute path to the Linux file that simulates the storage device
 *  \param p_bnmax pointer to a location where the number of blocks of the device is to be stored
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if \e devname or \e p_bnmax are \c NULL
 *  \return -\c EBUSY, if the device is already opened
 *  \return -\c ELIBBAD, if the supporting file size is invalid
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soOpenDevice (const char *devname, uint32_t *p_bnmax);

/**
 *  \brief Close the storage device.
 *
 *  The communication channel previously established with the storage device is closed.
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EBADF, if the device is not opened
 */

extern int soCloseDevice (void);

/**
 *  \brief Read a block of data from the storage device.
 *
 *  The device is organized as a linear array of data blocks.
 *  Both the physical number of the data block to be read and a pointer to a previously allocated buffer are supplied
 *  as arguments.
 *
 *  \param n physical number of the data block to be read from
 *  \param buf pointer to the buffer where the data must be read into
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>buffer pointer</em> is \c NULL or the <em>block number</em> is out of range
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on reading
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soReadRawBlock (uint32_t n, void *buf);

/**
 *  \brief Write a block of data from the storage device.
 *
 *  The device is organized as a linear array of data blocks.
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
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soWriteRawBlock (uint32_t n, void *buf);

/**
 *  \brief Read a cluster of data from the storage device.
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
 *  \return -\c EIO, if it fails on reading
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soReadRawCluster (uint32_t n, void *buf);

/**
 *  \brief Write a cluster of data from the storage device.
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
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soWriteRawCluster (uint32_t n, void *buf);

#endif /* SOFS_RAWDISK_H_ */
