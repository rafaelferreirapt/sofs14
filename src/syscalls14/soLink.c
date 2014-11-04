/**
 *  \file soLink.c (implementation file)
 *
 *  \author
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>
#include <libgen.h>
#include <string.h>

#include "sofs_probe.h"
#include "sofs_const.h"
#include "sofs_rawdisk.h"
#include "sofs_buffercache.h"
#include "sofs_superblock.h"
#include "sofs_inode.h"
#include "sofs_direntry.h"
#include "sofs_datacluster.h"
#include "sofs_basicoper.h"
#include "sofs_basicconsist.h"
#include "sofs_ifuncs_1.h"
#include "sofs_ifuncs_2.h"
#include "sofs_ifuncs_3.h"
#include "sofs_ifuncs_4.h"

/**
 *  \brief Make a new name for a file.
 *
 *  It tries to emulate <em>link</em> system call.
 *
 *  \param oldPath path to an existing file
 *  \param newPath new path to the same file
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to either of the strings is \c NULL or any of the path strings is a \c NULL string
 *                      or the path strings do not describe absolute paths
 *  \return -\c ENAMETOOLONG, if the paths names or any of their components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of both paths, but the last one, are not directories
 *  \return -\c ELOOP, if either path resolves to more than one symbolic link
 *  \return -\c ENOENT,  if no entry with a name equal to any of the components of <tt>oldPath</tt>, or to any of the
 *                       components of <tt>newPath</tt>, but the last one, is found
 *  \return -\c EEXIST, if a file described by <tt>newPath</tt> already exists
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of both paths, but the last one
 *  \return -\c EPERM, if the process that calls the operation has not write permission on the directory where
 *                     <tt>newPath</tt> entry is to be added, or <tt>oldPath</tt> represents a directory
 *  \return -\c ENOSPC, if there are no free data clusters
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soLink (const char *oldPath, const char *newPath)
{
  soColorProbe (225, "07;31", "soLink (\"%s\", \"%s\")\n", oldPath, newPath);

  /* insert your code here */

  return -ENOSYS;
}
