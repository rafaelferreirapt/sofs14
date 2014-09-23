/**
 *  \file sofs_rawdisk.c (implementation file)
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
 */

#include <sys/stat.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <errno.h>
#define __USE_GNU
#include <fcntl.h>

#include "sofs_const.h"
#include "sofs_probe.h"

/*
 *  Internal data structure
 */
/** \brief File descriptor of the Linux file that simulates the magnetic disk */
static int fd = -1;
/** \brief Number of blocks of the storage device */
static uint32_t bnmax = 0;

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

int soOpenDevice (const char *devname, uint32_t *p_bnmax)
{
  soColorProbe (851, "07;31", "soOpenDevice(\"%s\", %p)\n", devname, p_bnmax);

  if ((devname == NULL) || (p_bnmax == NULL))
     return -EINVAL;                             /* checking for null pointers */
  if (fd != -1) return -EBUSY;                   /* checking for device open state */

  /* opening supporting file in async mode for read and write */

  if ((fd = open (devname, O_RDWR)) == -1)
     return -errno;                              /* checking for opening error */

  /* checking device for conformity */

  struct stat st;
  if (stat (devname, &st) == -1) return -errno;
  if ((st.st_size % BLOCK_SIZE) != 0) return -ELIBBAD;

  bnmax = st.st_size / BLOCK_SIZE;               /* get number of blocks of the device */
  *p_bnmax = bnmax;

  return 0;
}

/**
 *  \brief Close the storage device.
 *
 *  The communication channel previously established with the storage device is closed.
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EBADF, if the device is not opened
 */

int soCloseDevice (void)
{
  soColorProbe (852, "07;31", "soCloseDevice()\n");

  if (fd == -1) return -EBADF;                   /* checking for device close state */

  close (fd);                                    /* close the device */
  bnmax = 0;                                     /* reset number of blocks of the storage device */
  fd = -1;                                       /* reset file descriptor of the Linux file that simulates the
                                                    magnetic disk */

  return 0;
}

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

int soReadRawBlock (uint32_t n, void *buf)
{
  soColorProbe (853, "07;31", "soReadRawBlock(%"PRIu32", %p)\n", n, buf);

  if (buf == NULL) return -EINVAL;               /* checking for null pointer */
  if (n >= bnmax) return -EINVAL;                /* checking for block number */
  if (fd == -1) return -EBADF;                   /* checking for device closed state */

  /* set file current position to the required block and read its contents */

  if (lseek (fd, BLOCK_SIZE * n, SEEK_SET) == -1) return -errno;
  if (read (fd, buf, BLOCK_SIZE) != BLOCK_SIZE) return -EIO;

  return 0;
}

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

int soWriteRawBlock (uint32_t n, void *buf)
{
  soColorProbe (854, "07;31", "soWriteRawBlock(%"PRIu32", %p)\n", n, buf);

  if (buf == NULL) return -EINVAL;               /* checking for null pointer */
  if (n >= bnmax) return -EINVAL;                /* checking for block number */
  if (fd == -1) return -EBADF;                   /* checking for device closed state */

  /* set file current position to the required block and write its contents */

  if (lseek (fd, BLOCK_SIZE * n, SEEK_SET) == -1) return -errno;
  if (write (fd, buf, BLOCK_SIZE) != BLOCK_SIZE) return -EIO;

  return 0;
}

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

int soReadRawCluster (uint32_t n, void *buf)
{
  soColorProbe (855, "07;31", "soReadRawCluster(%"PRIu32", %p)\n", n, buf);

  if (buf == NULL) return -EINVAL;               /* checking for null pointer */
  if ((n + BLOCKS_PER_CLUSTER) > bnmax)          /* checking for cluster number */
     return -EINVAL;
  if (fd == -1) return -EBADF;                   /* checking for device closed state */

  /* Set file current position to first block of the required cluster and read blocks contents in succession */

  if (lseek (fd, BLOCK_SIZE * n, SEEK_SET) == -1) return -errno;
  if (read (fd, buf, CLUSTER_SIZE) != CLUSTER_SIZE) return -EIO;

  return 0;
}

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

int soWriteRawCluster (uint32_t n, void *buf)
{
  soColorProbe (856, "07;31", "soWriteRawCluster(%"PRIu32", %p)\n", n, buf);

  if (buf == NULL) return -EINVAL;               /* checking for null pointer */
  if ((n + BLOCKS_PER_CLUSTER) > bnmax)          /* checking for cluster number */
     return -EINVAL;
  if (fd == -1) return -EBADF;                   /* checking for device closed state */

  /* Set file current position to first block of the required cluster and write blocks contents in succession */

  if (lseek (fd, BLOCK_SIZE * n, SEEK_SET) == -1) return -errno;
  if (write (fd, buf, CLUSTER_SIZE) != CLUSTER_SIZE) return -EIO;

  return 0;
}
