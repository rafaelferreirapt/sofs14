/**
 *  \file soFreeInode.c (implementation file)
 *
 *  \author
 */

#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#include "sofs_probe.h"
#include "sofs_buffercache.h"
#include "sofs_superblock.h"
#include "sofs_inode.h"
#include "sofs_datacluster.h"
#include "sofs_basicoper.h"
#include "sofs_basicconsist.h"

/**
 *  \brief Free the referenced inode.
 *
 *  The inode must be in use, belong to one of the legal file types and have no directory entries associated with it
 *  (refcount = 0).
 *  The inode is marked free in the dirty state and inserted in the list of free inodes.
 *
 *  Notice that the inode 0, supposed to belong to the file system root directory, can not be freed.
 *
 *  The only affected fields are:
 *     \li the free flag of mode field, which is set
 *     \li the <em>time of last file modification</em> and <em>time of last file access</em> fields, which change their
 *         meaning: they are replaced by the <em>prev</em> and <em>next</em> pointers in the double-linked list of free
 *         inodes.
 * *
 *  \param nInode number of the inode to be freed
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>inode number</em> is out of range
 *  \return -\c EIUININVAL, if the inode in use is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c EDCINVAL, if the data cluster header is inconsistent
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soFreeInode (uint32_t nInode)
{
   soColorProbe (612, "07;31", "soFreeInode (%"PRIu32")\n", nInode);

  /* insert your code here */

   return 0;
}
