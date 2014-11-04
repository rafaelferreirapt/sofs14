/**
 *  \file sofs_syscalls.h (interface file)
 *
 *  \brief Set of operations to manage system calls.
 *
 *  The aim is to provide an unique description of the functions that operate at this level.
 *
 *  The operations are:
 *      \li mount the SOFS14 file system
 *      \li unmount the SOFS14 file system
 *      \li get file system statistics
 *      \li get file status
 *      \li check real user's permissions for a file
 *      \li change permissions of a file
 *      \li change the ownership of a file
 *      \li change the last access and modification times of a file
 *      \li change the last access and modification times of a file with nanosecond resolution
 *      \li make a new name for a file
 *      \li delete the name of a file from a directory and possibly the file it refers to from the file system
 *      \li change the name or the location of a file in the directory hierarchy of the file system
 *      \li create a regular file with size 0
 *      \li open a regular file
 *      \li close a regular file
 *      \li read data from an open regular file
 *      \li write data into an open regular file
 *      \li truncate a regular file to a specified length
 *      \li synchronize a file's in-core state with storage device
 *      \li create a directory
 *      \li delete a directory
 *      \li open a directory for reading
 *      \li read a directory entry from a directory
 *      \li close a directory
 *      \li make a new name for a regular file or a directory
 *      \li read the value of a symbolic link.
 *
 *  \author Artur Carneiro Pereira September 2007
 *  \author Miguel Oliveira e Silva September 2009
 *  \author António Rui Borges - October 2010 / October 2014
 */

#ifndef SOFS_SYSCALLS_H_
#define SOFS_SYSCALLS_H_

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>
#include <libgen.h>

/**
 *  \brief Mount the SOFS14 file system.
 *
 *  A buffered communication channel is established with the storage device.
 *  The superblock is read and it is checked if the file system was properly unmounted the last time it was mounted. If
 *  not, a consistency check is performed (presently, the check is superficial, a more thorough one is required).
 *
 *  \param devname absolute path to the Linux file that simulates the storage device
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if if the pointer to <em>device path</em> is \c NULL or is a \c NULL string or the path does not
 *                      describe an absolute path or the magic number is not the one characteristic of SOFS14
 *  \return -\c ENAMETOOLONG, if the absolute path exceeds the maximum allowed length
 *  \return -\c EBUSY, if the storage area is already in use or the device is already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soMountSOFS (const char *devname);

/**
 *  \brief Unmount the SOFS14 file system.
 *
 *  The buffered communication channel previously established with the storage device is closed. This means, namely,
 *  that the contents of the storage area is flushed into the storage device to keep data update. Before that, however,
 *  the mount flag of the superblock is set to <em>properly unmounted</em>.
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soUnmountSOFS (void);

/**
 *  \brief Get file system statistics.
 *
 *  It tries to emulate <em>statvfs</em> system call.
 *
 *  Information about a mounted file system is returned.
 *  It checks whether the calling process can access the file specified by the path.
 *
 *  \param ePath path to any file within the mounted file system
 *  \param st pointer to a statvfs structure
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if any of the pointers are \c NULL or the path string is a \c NULL string or the path does not
 *                      describe an absolute path
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT,  if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soStatFS (const char *ePath, struct statvfs *st);

/**
 *  \brief Get file status.
 *
 *  It tries to emulate <em>stat</em> system call.
 *
 *  Information about a specific file is returned.
 *  It checks whether the calling process can access the file specified by the path.
 *
 *  \param ePath path to the file
 *  \param st pointer to a stat structure
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if any of the pointers are \c NULL  or the path string is a \c NULL string or the path does not
 *                      describe an absolute path
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT,  if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soStat (const char *ePath, struct stat *st);

/**
 *  \brief Check real user's permissions for a file.
 *
 *  It tries to emulate <em>access</em> system call.
 *
 *  It checks whether the calling process can access the file specified by the path.
 *
 *  \param ePath path to the file
 *  \param opRequested operation to be performed:
 *                    F_OK (check if file exists)
 *                    a bitwise combination of R_OK, W_OK, and X_OK
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or the path string is a \c NULL string or the path does
 *                      not describe an absolute path or no operation of the defined class is described
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT,  if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one, or the operation is denied
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soAccess (const char *ePath, int opRequested);

/**
 *  \brief Change permissions of a file.
 *
 *  It tries to emulate <em>chmod</em> system call.
 *
 *  It changes the permissions of a file specified by the path.
 *
 *  \remark If the file is a symbolic link, its contents shall always be used to reach the destination file, so the
 *          permissions of a symbolic link can never be changed (they are set to rwx for <em>user</em>, <em>group</em>
 *          and <em>other</em> when the link is created and remain unchanged thereafter).
 *
 *  \param ePath path to the file
 *  \param mode permissions to be set:
 *                    a bitwise combination of S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH,
                      S_IXOTH
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or the path string is a \c NULL string or the path does
 *                      not describe an absolute path or no mode of the defined class is described
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT,  if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c EPERM, if the process that calls the operation is neither the file's owner, nor is <em>root</em>
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soChmod (const char *ePath, mode_t mode);

/**
 *  \brief Change the ownership of a file.
 *
 *  It tries to emulate <em>chown</em> system call.
 *
 *  It changes the ownership of a file specified by the path.
 *
 *  Only <em>root</em> may change the owner of a file. The file's owner may change the group if the specified group is
 *  one of the owner's supplementary groups.
 *
 *  \param ePath path to the file
 *  \param owner file user id (-1, if user is not to be changed)
 *  \param group file group id (-1, if group is not to be changed)
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or the path string is a \c NULL string or the path does
 *                      not describe an absolute path
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT,  if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c EPERM, if the process that calls the operation is neither the file's owner, nor is <em>root</em>, nor
 *                     the specified group is one of the owner's supplementary groups
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soChown (const char *ePath, uid_t owner, gid_t group);

/**
 *  \brief Change the last access and modification times of a file.
 *
 *  It tries to emulate <em>utime</em> system call.
 *
 *  \param ePath path to the file
 *  \param times pointer to a structure where the last access and modification times are passed, if \c NULL, the last
 *               access and modification times are set to the current time
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or the path string is a \c NULL string or the path does
 *                      not describe an absolute path
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT,  if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c EPERM, if the process that calls the operation is neither the file's owner, nor is <em>root</em>, or
 *                     has not write permission
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soUtime (const char *ePath, const struct utimbuf *times);

/**
 *  \brief Change the last access and modification times of a file with nanosecond resolution.
 *
 *  It tries to emulate <em>utimensat</em> system call.
 *
 *  \param ePath path to the file
 *  \param tv structure array where the last access, element of index 0, and modification, element of index 1, times
 *            are passed, if \c NULL, the last access and modification times are set to the current time
 *            if the <tt>tv_nsec</tt> field of one of the <tt>timespec</tt> structures has the special
 *            value \c UTIME_NOW, then the corresponding file timestamp is set to the current time
 *            if the <tt>tv_nsec</tt> field of one of the <tt>timespec</tt> structures has the special
 *            value \c UTIME_OMIT, then the corresponding file timestamp is left unchanged
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or the path string is a \c NULL string or the path does
 *                      not describe an absolute path
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT,  if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c EPERM, if the process that calls the operation is neither the file's owner, nor is <em>root</em>, or
 *                     has not write permission
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soUtimens (const char *ePath, const struct timespec tv[2]);

/**
 *  \brief Open a regular file.
 *
 *  It tries to emulate <em>open</em> system call.
 *
 *  \param ePath path to the file
 *  \param flags access modes to be used:
 *                    O_RDONLY, O_WRONLY, O_RDWR
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or the path string is a \c NULL string or the path does
 *                      not describe an absolute path or no access mode of the defined class is described
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT, if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EISDIR, if <tt>ePath</tt> represents a directory
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c EPERM, if the process that calls the operation has not the proper permission (read / write) on the file
 *                     described by <tt>ePath</tt>
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soOpen (const char *ePath, int flags);

/**
 *  \brief Close a regular file.
 *
 *  It tries to emulate <em>close</em> system call.
 *
 *  \param ePath path to the file
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or the path string is a \c NULL string or the path does
 *                      not describe an absolute path
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT, if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EISDIR, if <tt>ePath</tt> represents a directory
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soClose (const char *ePath);

/**
 *  \brief Synchronize a file's in-core state with storage device.
 *
 *  It tries to emulate <em>fsync</em> system call.
 *
 *  \param ePath path to the file
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or the path string is a \c NULL string or the path does
 *                      not describe an absolute path
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT, if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soFsync (const char *ePath);

/**
 *  \brief Open a directory for reading.
 *
 *  It tries to emulate <em>opendir</em> system call.
 *
 *  \param ePath path to the file
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or the path string is a \c NULL string or the path does
 *                      not describe an absolute path
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt> is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT, if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c EPERM, if the process that calls the operation has not read permission on the directory described by
 *                     <tt>ePath</tt>
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soOpendir (const char *ePath);

/**
 *  \brief Close a directory.
 *
 *  It tries to emulate <em>closedir</em> system call.
 *
 *  \param ePath path to the file
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or the path string is a \c NULL string or the path does
 *                      not describe an absolute path
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt> is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT, if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soClosedir (const char *ePath);

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

extern int soLink (const char *oldPath, const char *newPath);

/**
 *  \brief Delete the name of a file from a directory and possibly the file it refers to from the file system.
 *
 *  It tries to emulate <em>unlink</em> system call.
 *
 *  \param ePath path to the file to be deleted
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or or the path string is a \c NULL string or the path does
 *                      not describe an absolute path
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of the path, but the last one, is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT,  if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of the path, but the last one
 *  \return -\c EPERM, if the process that calls the operation has not write permission on the directory where
 *                     <tt>ePath</tt> entry is to be removed
 *  \return -\c EISDIR, if <tt>ePath</tt> represents a directory
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soUnlink (const char *ePath);

/**
 *  \brief Change the name or the location of a file in the directory hierarchy of the file system.
 *
 *  It tries to emulate <em>rename</em> system call.
 *
 *  \param oldPath path to an existing file
 *  \param newPath new path to the same file in replacement of the old one
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to either of the strings is \c NULL or any of the path strings is a \c NULL string
 *                      or any of the paths do not describe absolute paths or <tt>oldPath</tt> describes a directory
 *                      and is a substring of <tt>newPath</tt> (attempt to make a directory a subdirectory of itself)
 *  \return -\c ENAMETOOLONG, if the paths names or any of their components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of both paths, but the last one, are not directories, or
 *                       <tt>oldPath</tt> describes a directory and <tt>newPath</tt>, although it exists, does not
 *  \return -\c EISDIR, if <tt>newPath</tt> describes a directory and <tt>oldPath</tt> does not
 *  \return -\c ELOOP, if either path resolves to more than one symbolic link
 *  \return -\c EMLINK, if <tt>oldPath</tt> is a directory and the directory containing <tt>newPath</tt> has already
 *                      the maximum number of links, or <tt>oldPath</tt> has already the maximum number of links and
 *                      is not contained in the same directory that will contain <tt>newPath</tt>
 *  \return -\c ENOENT, if no entry with a name equal to any of the components of <tt>oldPath</tt>, or to any of the
 *                      components of <tt>newPath</tt>, but the last one, is found
 *  \return -\c ENOTEMPTY, if both <tt>oldPath</tt> and <tt>newPath</tt> describe directories and <tt>newPath</tt> is
 *                         not empty
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of both paths, but the last one
 *  \return -\c EPERM, if the process that calls the operation has not write permission on the directories where
 *                     <tt>newPath</tt> entry is to be added and <tt>oldPath</tt> is to be detached
 *  \return -\c ENOSPC, if there are no free data clusters
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soRename (const char *oldPath, const char *newPath);

/**
 *  \brief Create a regular file with size 0.
 *
 *  It tries to emulate <em>mknod</em> system call.
 *
 *  \param ePath path to the file
 *  \param mode type and permissions to be set:
 *                    a bitwise combination of S_IFREG, S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH,
 *                    S_IWOTH, S_IXOTH
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or or the path string is a \c NULL string or the path does
 *                      not describe an absolute path or no mode of the defined class is described
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT, if no entry with a name equal to any of the components of <tt>ePath</tt>, but the last one, is
 *                      found
 *  \return -\c EEXIST, if a file described by <tt>ePath</tt> already exists or the last component is a symbolic link
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c EPERM, if the process that calls the operation has not write permission on the directory that will hold
 *                     <tt>ePath</tt>
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soMknod (const char *ePath, mode_t mode);

/**
 *  \brief Read data from an open regular file.
 *
 *  It tries to emulate <em>read</em> system call.
 *
 *  \param ePath path to the file
 *  \param buff pointer to the buffer where data to be read is to be stored
 *  \param count number of bytes to be read
 *  \param pos starting [byte] position in the file data continuum where data is to be read from
 *
 *  \return <em>number of bytes effectively read</em>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or or the path string is a \c NULL string or the path does
 *                      not describe an absolute path
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c EISDIR, if <tt>ePath</tt> describes a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT, if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EFBIG, if the starting [byte] position in the file data continuum assumes a value passing its maximum
 *                     size
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c EPERM, if the process that calls the operation has not read permission on the file described by
 *                     <tt>ePath</tt>
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soRead (const char *ePath, void *buff, uint32_t count, int32_t pos);

/**
 *  \brief Write data into an open regular file.
 *
 *  It tries to emulate <em>write</em> system call.
 *
 *  \param ePath path to the file
 *  \param buff pointer to the buffer where data to be written is stored
 *  \param count number of bytes to be written
 *  \param pos starting [byte] position in the file data continuum where data is to be written into
 *
 *  \return <em>number of bytes effectively written</em>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or or the path string is a \c NULL string or the path does
 *                      not describe an absolute path
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c EISDIR, if <tt>ePath</tt> describes a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT, if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EFBIG, if the file may grow passing its maximum size
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c EPERM, if the process that calls the operation has not write permission on the file described by
 *                     <tt>ePath</tt>
 *  \return -\c ENOSPC, if there are no free data clusters
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soWrite (const char *ePath, void *buff, uint32_t count, int32_t pos);

/**
 *  \brief Truncate a regular file to a specified length.
 *
 *  It tries to emulate <em>truncate</em> system call.
 *
 *  \param ePath path to the file
 *  \param length new size for the regular size
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or or the path string is a \c NULL string or the path does
 *                      not describe an absolute path
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c EISDIR, if <tt>ePath</tt> describes a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT, if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EFBIG, if the file may grow passing its maximum size
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c EPERM, if the process that calls the operation has not write permission on the file described by
 *                     <tt>ePath</tt>
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soTruncate (const char *ePath, off_t length);

/**
 *  \brief Create a directory.
 *
 *  It tries to emulate <em>mkdir</em> system call.
 *
 *  \param ePath path to the file
 *  \param mode type and permissions to be set:
 *                    a bitwise combination of S_ISVTX, S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH,
 *                    S_IWOTH, S_IXOTH
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or or the path string is a \c NULL string or the path does
 *                      not describe an absolute path or no mode of the defined class is described
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt> is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT, if no entry with a name equal to any of the components of <tt>ePath</tt>, but the last one, is
 *                      found
 *  \return -\c EEXIST, if a file described by <tt>ePath</tt> already exists or the last component is a symbolic link
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c EPERM, if the process that calls the operation has not write permission on the directory that will hold
 *                     <tt>ePath</tt>
 *  \return -\c ENOSPC, if there are no free data clusters
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soMkdir (const char *ePath, mode_t mode);

/**
 *  \brief Delete a directory.
 *
 *  It tries to emulate <em>rmdir</em> system call.
 *
 *  \param ePath path to the directory to be deleted
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or the path string is a \c NULL string or the path does
 *                      not describe an absolute path
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ENOTDIR, if any of the components of the path is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT, if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c ENOTEMPTY, if <tt>ePath</tt> describes a non-empty directory
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of the path, but the last one
 *  \return -\c EPERM, if the process that calls the operation has not write permission on the directory where
 *                     <tt>ePath</tt> entry is to be removed
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soRmdir (const char *ePath);

/**
 *  \brief Read a directory entry from a directory.
 *
 *  It tries to emulate <em>getdents</em> system call, but it reads a single directory entry in use at a time.
 *
 *  Only the field <em>name</em> is read.
 *
 *  \remark The returned value is the number of bytes read from the directory in order to get the next in use
 *          directory entry. So, skipped free directory entries must be accounted for. The point is that the system
 *          (through FUSE) uses the returned value to update file position.
 *
 *  \param ePath path to the file
 *  \param buff pointer to the buffer where data to be read is to be stored
 *  \param pos starting [byte] position in the file data continuum where data is to be read from
 *
 *  \return <em>number of bytes effectively read to get a directory entry in use (0, if the end is reached)</em>,
 *          on success
 *  \return -\c EINVAL, if either of the pointers are \c NULL or or the path string is a \c NULL string or the path does
 *                      not describe an absolute path or <em>pos</em> value is not a multiple of the size of a
 *                      <em>directory entry</em>
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length
 *  \return -\c ERELPATH, if the path is relative
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt> is not a directory
 *  \return -\c EFBIG, if the <em>pos</em> value is beyond the maximum file size
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT, if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c EPERM, if the process that calls the operation has not read permission on the directory described by
 *                     <tt>ePath</tt>
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soReaddir (const char *ePath, void *buff, int32_t pos);

/**
 *  \brief Make a new name for a regular file or a directory.
 *
 *  It tries to emulate <em>symlink</em> system call.
 *
 *  \remark The permissions set for the symbolic link should have read (r), write (w) and execution (x) permissions for
 *          both <em>user</em>, <em>group</em> and <em>other</em>.
 *
 *  \param effPath path to be stored in the symbolic link file
 *  \param ePath path to the symbolic link
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the pointer to either of the strings is \c NULL or or any of the path strings is a \c NULL
 *                      string or the second string does not describe an absolute path
 *  \return -\c ENAMETOOLONG, if the either path names or any of its components exceed the maximum allowed length
 *  \return -\c ERELPATH, if the second path is relative
 *  \return -\c ENOTDIR, if any of the components of the second path, but the last one, is not a directory
 *  \return -\c ELOOP, if the second path resolves to more than one symbolic link
 *  \return -\c ENOENT, if no entry with a name equal to any of the components of <tt>ePath</tt>, but the last one, is
 *                      found
 *  \return -\c EEXIST, if a file described by <tt>ePath</tt> already exists
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of the second path, but the last one
 *  \return -\c EPERM, if the process that calls the operation has not write permission on the directory where
 *                     <tt>ePath</tt> entry is to be added
 *  \return -\c ENOSPC, if there are no free data clusters
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soSymlink (const char *effPath, const char *ePath);

/**
 *  \brief Read the value of a symbolic link.
 *
 *  It tries to emulate <em>readlink</em> system call.
 *
 *  \param ePath path to the symbolic link
 *  \param buff pointer to the buffer where data to be read is to be stored
 *  \param size buffer size in bytes
 *
 *  \return <em>number of bytes effectively read</em>, on success
 *  \return -\c EINVAL, if the pointer to the string is \c NULL or or the path string is a \c NULL string or the path does
 *                      not describe an absolute path or <tt>ePath</tt> does not represent a symbolic link
 *  \return -\c ENAMETOOLONG, if the path name or any of its components exceed the maximum allowed length or the buffer
 *                            size is not large enough
 *  \return -\c ERELPATH, if the path is relative
 *  \return -\c ENOTDIR, if any of the components of <tt>ePath</tt>, but the last one, is not a directory
 *  \return -\c ELOOP, if the path resolves to more than one symbolic link
 *  \return -\c ENOENT, if no entry with a name equal to any of the components of <tt>ePath</tt> is found
 *  \return -\c EACCES, if the process that calls the operation has not execution permission on any of the components
 *                      of <tt>ePath</tt>, but the last one
 *  \return -\c EPERM, if the process that calls the operation has not read permission on the symbolic link described by
 *                     <tt>ePath</tt>
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails on writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soReadlink (const char *ePath, const char *buff, int32_t size);

#endif /* SOFS_SYSCALLS_H_ */
