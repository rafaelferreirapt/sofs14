/**
 *  \file soAllocInode.c (implementation file)
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
/* #define  CLEAN_INODE */
#ifdef CLEAN_INODE
#include "sofs_ifuncs_2.h"
#endif

/**
 *  \brief Allocate a free inode.
 *
 *  The inode is retrieved from the list of free inodes, marked in use, associated to the legal file type passed as
 *  a parameter and generally initialized. It must be free and if is free in the dirty state, it has to be cleaned
 *  first.
 *
 *  Upon initialization, the new inode has:
 *     \li the field mode set to the given type, while the free flag and the permissions are reset
 *     \li the owner and group fields set to current userid and groupid
 *     \li the <em>prev</em> and <em>next</em> fields, pointers in the double-linked list of free inodes, change their
 *         meaning: they are replaced by the <em>time of last file modification</em> and <em>time of last file
 *         access</em> which are set to current time
 *     \li the reference fields set to NULL_CLUSTER
 *     \li all other fields reset.

 *  \param type the inode type (it must represent either a file, or a directory, or a symbolic link)
 *  \param p_nInode pointer to the location where the number of the just allocated inode is to be stored
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the <em>type</em> is illegal or the <em>pointer to inode number</em> is \c NULL
 *  \return -\c ENOSPC, if the list of free inodes is empty
 *  \return -\c EFININVAL, if the free inode is inconsistent
 *  \return -\c EFDININVAL, if the free inode in the dirty state is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c EDCINVAL, if the data cluster header is inconsistent
 *  \return -\c EWGINODENB, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em>
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soAllocInode (uint32_t type, uint32_t* p_nInode)
{
   soColorProbe (611, "07;31", "soAllocInode (%"PRIu32", %p)\n", type, p_nInode);

  /* insert your code here */

   return 0;
}
