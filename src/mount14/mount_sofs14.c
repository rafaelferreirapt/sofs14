/**
 *  \file mount_sofs14.c (implementation file)
 *
 *  \brief The SOFS14 mounting tool.
 *
 *  It provides a simple method to integrate the SOFS14 file system into Linux.
 *
 *  SINOPSIS:
 *  <P><PRE>                mount_sofs14 [OPTIONS] supp-file mount-point
 *
 *               OPTIONS:
 *                 -d       --- set debugging mode (default: no debugging)
 *                 -l depth --- set log depth (default: 0,0)
 *                 -L file  --- log file (default: stdout)
 *                 -h       --- print this help.</PRE>
 *
 *  \author Artur Carneiro Pereira - October 2005
 *  \author Miguel Oliveira e Silva - September 2009
 *  \author João Rodrigues - September 2009
 *  \author António Rui Borges - October 2010 / October 2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <libgen.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <fuse.h>
#include <fuse/fuse.h>

#include "sofs_probe.h"
#include "sofs_const.h"
#include "sofs_direntry.h"
#include "sofs_syscalls.h"

/*
 *  Access with mutual exclusion to some of the operations
 */

static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;                                         /* locking flag */

/*
 *  Allusion to FUSE callbacks and other internal functions
 */

static void *sofs_mount (struct fuse_conn_info *fci);
static void sofs_unmount (void *ePath);
static int sofs_statfs (const char *ePath, struct statvfs *st);
static int sofs_getattr (const char *ePath, struct stat *st);
static int sofs_access (const char *ePath, int mode);
static int sofs_utime (const char *ePath, struct utimbuf *times);
static int sofs_chmod (const char *ePath, mode_t mode);
static int sofs_chown (const char *ePath, uid_t owner, gid_t group);
static int sofs_mknod (const char *ePath, mode_t mode, dev_t rdev);
static int sofs_open (const char *ePath, struct fuse_file_info *fi);
static int sofs_read (const char *ePath, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int sofs_write (const char *ePath, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int sofs_flush (const char *ePath, struct fuse_file_info *fi);
static int sofs_release (const char *ePath, struct fuse_file_info *fi);
static int sofs_mkdir (const char *ePath, mode_t mode);
static int sofs_rmdir (const char *ePath);
static int sofs_opendir (const char *ePath, struct fuse_file_info *fi);
static int sofs_readdir (const char *ePath, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int sofs_getdir (const char *ePath, fuse_dirh_t handle, fuse_dirfil_t filler);
static int sofs_releasedir (const char *ePath, struct fuse_file_info *fi);
static int sofs_link (const char *oldPath, const char *newPath);
static int sofs_unlink (const char *ePath);
static int sofs_rename (const char *oldPath, const char *newPath);
static int sofs_truncate (const char *ePath, off_t length);
static int sofs_readlink (const char *ePath, char *buf, size_t size);
static int sofs_symlink (const char *effPath, const char *ePath);
static int sofs_fsync (const char *ePath, int, struct fuse_file_info *fi);
static int sofs_fsyncdir (const char *ePath, int, struct fuse_file_info *fi);
static int sofs_setxattr (const char *ePath, const char *name, const char *value, size_t size, int flags);
static int sofs_getxattr (const char *ePath, const char *name, char *value, size_t size);
static int sofs_listxattr (const char *ePath, char *list, size_t size);
static int sofs_removexattr (const char *ePath, const char *name);
static void printUsage (char *cmd_name);

/*
 *  Set of FUSE operations (required by the FUSE filesystem)
 */

static struct fuse_operations fuse_operations = {.getattr     = sofs_getattr,
                                                 .readlink    = sofs_readlink,
                                                 .getdir      = sofs_getdir,        /* deprecated */
                                                 .mknod       = sofs_mknod,
                                                 .mkdir       = sofs_mkdir,
                                                 .unlink      = sofs_unlink,
                                                 .rmdir       = sofs_rmdir,
                                                 .symlink     = sofs_symlink,
                                                 .rename      = sofs_rename,
                                                 .link        = sofs_link,
                                                 .chmod       = sofs_chmod,
                                                 .chown       = sofs_chown,
                                                 .truncate    = sofs_truncate,
                                                 .utime       = sofs_utime,         /* deprecated */
                                                 .open        = sofs_open,
                                                 .read        = sofs_read,
                                                 .write       = sofs_write,
                                                 .statfs      = sofs_statfs,
                                                 .flush       = sofs_flush,
                                                 .release     = sofs_release,
                                                 .fsync       = sofs_fsync,
                                                 .setxattr    = sofs_setxattr,
                                                 .getxattr    = sofs_getxattr,
                                                 .listxattr   = sofs_listxattr,
                                                 .removexattr = sofs_removexattr,
                                                 .opendir     = sofs_opendir,
                                                 .readdir     = sofs_readdir,
                                                 .releasedir  = sofs_releasedir,
                                                 .fsyncdir    = sofs_fsyncdir,
                                                 .init        = sofs_mount,
                                                 .destroy     = sofs_unmount,
                                                 .access      = sofs_access,
                                                 .create      = NULL,
                                                 .ftruncate   = NULL,
                                                 .fgetattr    = NULL,
                                                 .lock        = NULL,
                                                 .utimens     = NULL,
                                                 .bmap        = NULL,
                                                 .flag_nullpath_ok = 0,
                                                 .flag_reserved = 0 ,
                                                 .ioctl       = NULL,
                                                 .poll        = NULL
                                                };

/* SOFS10 support filename (should be the absolute path) */

static char *sofs_supp_file = NULL;

/* The main function */

int main(int argc, char *argv[])
{
  int lower = 0;                                 /* lower limit of log depth, if kept set to zero */
  int higher = 0;                                /* upper limit of log depth, if kept set to zero */
  int debug_mode = 0;                            /* debugging mode, if kept set to zero */
  FILE *fl = NULL;                               /* log stream default */

  /* process command line options */

  int opt;                                       /* selected option */

  do
  { switch ((opt = getopt (argc, argv, "l:L:dh")))
    { case 'l': /* log depth */
                if (sscanf (optarg, "%d,%d", &lower, &higher) != 2)
                   { fprintf (stderr, "%s: Bad argument to l option.\n", basename (argv[0]));
                     printUsage (basename (argv[0]));
                     return EXIT_FAILURE;
                }
                soSetProbe (lower, higher);
                break;
      case 'L': /* log file */
                if ((fl = fopen (optarg, "w")) == NULL)
                   { fprintf (stderr, "%s: Can't open log file \"%s\".\n", basename (argv[0]), optarg);
                     printUsage (basename (argv[0]));
                     return EXIT_FAILURE;
                   }
                soOpenProbe (fl);
                break;
      case 'd': /* debugging mode */
                debug_mode = 1;                  /* set debugging mode for processing: no FUSE messages are issued */
                break;
      case 'h': /* help mode */
                printUsage (basename (argv[0]));
                return EXIT_SUCCESS;
      case -1:  break;
      default:  fprintf (stderr, "%s: Wrong option.\n", basename (argv[0]));
                printUsage (basename (argv[0]));
                return EXIT_FAILURE;
    }
  } while (opt != -1);
  if ((argc - optind) != 2)                      /* check existence of mandatory argument: storage device name */
     { fprintf (stderr, "%s: Wrong number of mandatory arguments.\n", basename (argv[0]));
       printUsage (basename (argv[0]));
       return EXIT_FAILURE;
     }

  /* set the absolute path for the storage device name */

  if ((sofs_supp_file = realpath(argv[optind], NULL)) == NULL)
     { fprintf (stderr, "%s: Setting the absolute path - %s.\n", basename (argv[0]), strerror (errno));
       return EXIT_FAILURE;
     }

  if (fl == NULL)
     fl = stdout;                                /* if the switch -L was not used, set output to stdout */
     else stderr = fl;                           /* if the switch -L was used, set stderr to log file */

  /* build argv and argc for fuse_main */

  char *fuse_argv[] = { argv[0], argv[optind+1],
                        /* "-s", */
                        "-o", "nonempty",
                        "-o", "fsname=SOFS14", "-o", "subtype=ext-like",
                        /* "-o", "entry_timeout=0", "-o", "attr_timeout=0", */
                        "-d"
                      };
  int fuse_argc = (debug_mode) ? 9 : 8;

  return fuse_main (fuse_argc, fuse_argv, &fuse_operations, NULL);
}

/*
 * print help message
 */

static void printUsage (char *cmd_name)
{
  printf ("Sinopsis: %s [OPTIONS] supp-file mount-point\n"
          "  OPTIONS:\n"
          "  -d       --- set debugging mode (default: no debugging)\n"
          "  -l depth --- set log depth (default: 0,0)\n"
          "  -L file  --- log file (default: stdout)\n"
          "  -h       --- print this help\n", cmd_name);
}

/* Functions to be implemented */

/**
 *  \brief Mount the filesystem.
 *
 *  The return value will be passed in the private_data field of fuse_context to all file operations and as a parameter
 *  to the destroy () method.
 *
 *  \remarks Introduced in version 2.3 and changed in version 2.6.
 *
 *  \param fci pointer to fuse connection information
 *
 *  \return pointer to the path of the support file
 */

static void *sofs_mount (struct fuse_conn_info* fci)
{
  soColorProbe (111, "07;31", "sofs_mount_bin ()\n");

  int stat;

  if ((stat = soMountSOFS (sofs_supp_file)) != 0) return NULL;
  return sofs_supp_file;
}

/**
 *  \brief Unmount the filesystem.
 *
 *  Called on filesystem exit.
 *
 *  \remarks Introduced in version 2.3.
 *
 *  \param path pointer to the path of the support file
 */

static void sofs_unmount (void *path)
{
  soColorProbe (112, "07;31", "sofs_unmount_bin (\"%s\")\n", (char *) path);

  pthread_mutex_lock (&accessCR);                                    /* enter critical region */

  soUnmountSOFS ();

  pthread_mutex_unlock (&accessCR);                                  /* exit critical region */
}

/**
 *  \brief Get file status.
 *
 *  Similar to stat (man 2 stat).
 *
 *  The 'st_dev' and 'st_blksize' fields are ignored. The 'st_ino' field is ignored except if the 'use_ino'
 *  mount option is given.
 *
 *  \param ePath pointer to path
 *  \param st pointer to stat structure
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_getattr (const char *ePath, struct stat *st)
{
  soColorProbe (113, "07;31", "sofs_getattr_bin (\"%s\", %p)\n", ePath, st);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soStat (ePath, st);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  Check file access permissions.
 *
 *  Equivalent to the system call access (man 2 access).
 *
 *  \remarks Introduced in version 2.5.
 *
 *  \param ePath pointer to path
 *  \param opRequested operation to be performed
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_access (const char *ePath, int opRequested)
{
  soColorProbe (114, "07;31", "sofs_access_bin (\"%s\", %x)\n", ePath, opRequested);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soAccess (ePath, opRequested);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  \brief Create a file node.
 *
 *  Similar to system call mknod (man 2 mknod).
 *
 *  This is called for creation of all non-directory, non-symlink nodes. If the filesystem defines a
 *  create () method, then for regular files that will be called instead.
 *
 *  \param ePath path to the file
 *  \param mode type and permissions to be set
 *  \param rdev raw device id
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_mknod (const char *ePath, mode_t mode, dev_t rdev)
{
  soColorProbe (115, "07;31", "sofs_mknod_bin (\"%s\", %x, %x)\n", ePath, (uint32_t) mode, (uint32_t) rdev);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soMknod (ePath, mode);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  \brief Create a directory.
 *
 *  Similar to system call mkdir (man 2 mkdir).
 *
 *  Note that the mode argument may not have the type specification bits set, i.e. S_ISDIR(mode) can be
 *  false. To obtain the correct directory type bits use mode | S_IFDIR.
 *
 *  \param ePath path to the file
 *  \param mode type and permissions to be set
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_mkdir (const char *ePath, mode_t mode)
{
  soColorProbe (116, "07;31", "sofs_mkdir_bin (\"%s\", %x)\n", ePath, (uint32_t) mode);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soMkdir (ePath, mode | S_IFDIR);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  \brief Remove a regular file.
 *
 *  Similar to system call unlink (man 2 unlink).
 *
 *  \param ePath path to the file
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_unlink (const char *ePath)
{
  soColorProbe (117, "07;31", "sofs_unlink_bin (\"%s\")\n", ePath);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soUnlink (ePath);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  \brief Remove a directory.
 *
 *  Similar to system call rmdir (man 2 rmdir).
 *
 *  \param ePath path to the file
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_rmdir (const char *ePath)
{
  soColorProbe (118, "07;31", "sofs_rmdir_bin (\"%s\")\n", ePath);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soRmdir (ePath);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  \brief Rename a file.
 *
 *  Similar to system call rename (man 2 rename).
 *
 *  \param oldPath path to an existing file
 *  \param newPath new path to the same file in replacement of the old one
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_rename (const char *oldPath, const char *newPath)
{
  soColorProbe (119, "07;31", "sofs_rename_bin (\"%s\", \"%s\")\n", oldPath, newPath);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soRename (oldPath, newPath);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/** \brief Create a hard link to a file.
 *
 *  Similar to system call link (man 2 link).
 *
 *  \param oldPath path to an existing file
 *  \param newPath new path to the same file
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_link (const char *oldPath, const char *newPath)
{
  soColorProbe (120, "07;31", "sofs_link_bin (\"%s\", \"%s\")\n", oldPath, newPath);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soLink (oldPath, newPath);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/** \brief Change the permission bits of a file.
 *
 *  Similar to system call chmod (man 2 chmod).
 *
 *  \param ePath path to the file
 *  \param mode permissions to be set
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_chmod (const char *ePath, mode_t mode)
{
  soColorProbe (121, "07;31", "sofs_chmod_bin (\"%s\", 0%o)\n", ePath, (uint32_t) mode);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soChmod (ePath, mode);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/** \brief Change the owner and group of a file.
 *
 *  Similar to system call chown (man 2 chown).
 *
 *  \param ePath path to the file
 *  \param owner file user id (-1, if user is not to be changed)
 *  \param group file group id (-1, if group is not to be changed)
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_chown (const char *ePath, uid_t owner, gid_t group)
{
  soColorProbe (122, "07;31", "sofs_chown_bin (\"%s\", %"PRIu32", %"PRIu32")\n", ePath, (uint32_t) owner,
		        (uint32_t) group);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soChown (ePath, owner, group);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/** \brief Change the length of a file.
 *
 *  Similar to system call truncate (man 2 truncate).
 *
 *  \param ePath path to the file
 *  \param length new size for the regular size
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_truncate (const char *ePath, off_t length)
{
  soColorProbe (123, "07;31", "sofs_truncate_bin (\"%s\", %u)\n", ePath, (uint32_t) length);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soTruncate (ePath, length);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/** \brief Change the access and/or modification times of a file.
 *
 *  Similar to system call utime (man 2 utime).
 *
 *  Change the access and modification times of a file with nanosecond resolution.
 *
 *  \param ePath path to the file
 *  \param times pointer to a structure where the last access and modification times are passed, if \c NULL, the last
 *               access and modification times are set to the current time
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_utime (const char *ePath, struct utimbuf *times)
{
  soColorProbe (124, "07;31", "sofs_utime_bin (\"%s\", %p)\n", ePath, times);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soUtime (ePath, times);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  \brief Get file system statistics.
 *
 *  Equivalent to system call statvfs (man 2 statvfs).
 *
 *  The 'f_type' and 'f_fsid' fields are ignored.
 *
 *  \param ePath path to any file within the mounted file system
 *  \param st pointer to a statvfs structure
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_statfs (const char *ePath, struct statvfs *st)
{
  soColorProbe (125, "07;31", "sofs_statfs_bin (\"%s\", %p)\n", ePath, st);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soStatFS (ePath, st);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  \brief File open operation.
 *
 *  Equivalent to system call open (man 2 open).
 *
 *  No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC) will be passed to open().  Open should check if the
 *  operation is permitted for the given flags.  Optionally open may also return an arbitrary filehandle in the
 *  fuse_file_info structure, which will be passed to all file operations.
 *
 *  \remarks Changed in version 2.2.
 *
 *  \param ePath path to the file
 *  \param fi pointer to fuse file information
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_open (const char *ePath, struct fuse_file_info *fi)
{
  soColorProbe (126, "07;31", "sofs_open_bin (\"%s\", %p)\n", ePath, fi);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soOpen (ePath, fi->flags);
  fi->fh = (uint64_t) 0;

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  \brief Read data from an open file.
 *
 *  Equivalent to system call read (man 2 read).
 *
 *  Read should return exactly the number of bytes requested except on EOF or error, otherwise the rest of the data
 *  will be substituted with zeroes.  An exception to this is when the 'direct_io' mount option is specified, in which
 *  case the return value of the read system call will reflect the return value of this operation.
 *
 *  \remarks Changed in version 2.2.
 *
 *  \param ePath path to the file
 *  \param buff pointer to the buffer where data to be read is to be stored
 *  \param count number of bytes to be read
 *  \param pos starting [byte] position in the file data continuum where data is to be read from
 *  \param fi pointer to fuse file information
 *
 *  \return number of bytes effectively read, on success, and a negative value, on error
 */

static int sofs_read (const char *ePath, char *buff, size_t count, off_t pos, struct fuse_file_info *fi)
{
  soColorProbe (127, "07;31", "sofs_read_bin (\"%s\", %p, %"PRIu32", %"PRId32", %p)\n", ePath, buff, (uint32_t) count,
                (int32_t) pos, fi);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soRead (ePath, buff, (uint32_t) count, (int32_t) pos);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  \brief Write data to an open file.
 *
 *  Equivalent to system call write (man 2 write).
 *
 *  Write should return exactly the number of bytes requested except on error.  An exception to this is when the
 *  'direct_io' mount option is specified (see read operation).
 *
 *  \remarks Changed in version 2.2.
 *
 *  \param ePath path to the file
 *  \param buff pointer to the buffer where data to be written is stored
 *  \param count number of bytes to be read
 *  \param pos starting [byte] position in the file data continuum where data is to be read from
 *  \param fi pointer to fuse file information
 *
 *  \return number of bytes effectively written, on success, and a negative value, on error
 */

static int sofs_write (const char *ePath, const char *buff, size_t count, off_t pos, struct fuse_file_info *fi)
{
  soColorProbe (128, "07;31", "sofs_write_bin (\"%s\", %p, %"PRIu32", %"PRId32", %p)\n", ePath, buff, (uint32_t) count,
                (int32_t) pos, fi);

  int i;
  int stat;
  char *b;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  b = malloc (count);
  for (i = 0; i < count; i++)
    b[i] = buff[i];
  stat = soWrite (ePath, (void *) b, (uint32_t) count, (int32_t) pos);
  free (b);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  \brief Possibly flush cached data.
 *
 *  Equivalent to system call close (man 2 close).
 *
 *  BIG NOTE: This is not equivalent to fsync(). It's not a request to sync dirty data.
 *
 *  Flush is called on each close() of a file descriptor. So if a filesystem wants to return write errors in close()
 *  and the file has cached dirty data, this is a good place to write back data and return any errors.  Since many
 *  applications ignore close() errors this is not always useful.
 *
 *  NOTE: The flush() method may be called more than once for each  open().  This happens if more than one file
 *  descriptor refers to an opened file due to dup(), dup2() or fork() calls.  It is not possible to determine if a
 *  flush is final, so each flush should be treated equally.  Multiple write-flush sequences are relatively rare, so
 *  this shouldn't be a problem.
 *
 *  Filesystems shouldn't assume that flush will always be called after some writes, or that if will be called at all.
 *
 *  \remarks Changed in version 2.2.
 *
 *  \param ePath path to the file
 *  \param fi pointer to fuse file information
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_flush (const char *ePath, struct fuse_file_info *fi)
{
  soColorProbe(129, "07;31", "sofs_flush_bin (\"%s\", %p)\n", ePath, fi);

  return 0;
}

/**
 *  \brief Release an open file.
 *
 *  Release is called when there are no more references to an open file: all file descriptors are closed and all memory
 *  mappings are unmapped.
 *
 *  For every open() call there will be exactly one release() call with the same flags and file descriptor. It is
 *  possible to have a file opened more than once, in which case only the last release will mean, that no more
 *  reads/writes will happen on the file. The return value of release is ignored.
 *
 *  \remarks Changed in version 2.2.
 *
 *  \param ePath path to the file
 *  \param fi pointer to fuse file information
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_release (const char *ePath, struct fuse_file_info *fi)
{
  soColorProbe (130, "07;31", "sofs_release_bin (\"%s\", %p)\n", ePath, fi);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soClose (ePath);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  \brief Synchronize file contents.
 *
 *  Equivalent to system calls fsync and fdatasync (man 2 fsync and fdatasync).
 *
 *  If the datasync parameter is non-zero, then only the user data should be flushed, not the meta data.
 *
 *  \remarks Changed in version 2.2.
 *
 *  \param ePath path to the file
 *  \param isdatasync flag signaling if only the user data should be flushed
 *  \param fi pointer to fuse file information
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_fsync (const char *ePath, int isdatasync, struct fuse_file_info *fi)
{
  soColorProbe(131, "07;31", "sofs_fsync_bin (\"%s\", %d, %p)\n", ePath, isdatasync, fi);

  return soFsync (ePath);
}

/**
 *  \brief Open directory.
 *
 *  Equivalent to opendir (man 3 opendir).
 *
 *  This method should check if the open operation is permitted for this  directory
 *
 *  \remarks Introduced in version 2.3.
 *
 *  \param ePath path to the file
 *  \param fi pointer to fuse file information
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_opendir (const char *ePath, struct fuse_file_info *fi)
{
  soColorProbe (132, "07;31", "sofs_opendir_bin (\"%s\", %p)\n", ePath, fi);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soOpendir (ePath);
  fi->fh = (uint64_t) 0;

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  \brief Read directory.
 *
 *  Equivalent to readdir (man 3 readdir).
 *
 *  This supersedes the old getdir() interface.  New applications should use this.
 *
 *  The filesystem may choose between two modes of operation:
 *
 *  1) The readdir implementation ignores the offset parameter, and passes zero to the filler function's offset. The
 *     filler function will not return '1' (unless an error happens), so the whole directory is read in a single
 *     readdir operation. This works just like the old getdir() method.
 *
 *  2) The readdir implementation keeps track of the offsets of the directory entries.  It uses the offset parameter
 *     and always passes non-zero offset to the filler function.  When the buffer is full (or an error happens) the
 *     filler function will return '1'.
 *
 *  \remarks Introduced in version 2.3.
 *
 *  \param ePath path to the file
 *  \param buf pointer to the buffer where data to be read is to be stored
 *  \param filler pointer to the filler function
 *  \param offset starting [byte] position in the file data continuum where data is to be read from
 *  \param fi pointer to fuse file information
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_readdir (const char *ePath, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
  soColorProbe (133, "07;31", "sofs_readdir_bin (\"%s\", %p, %p, %"PRId32", %p)\n", ePath, buf, filler,
                (int32_t)offset, fi);

  char name[MAX_NAME+1];
  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soReaddir (ePath, name, (int32_t) offset);
  if (stat > 0)
     { offset += stat;
       stat = filler (buf, name, NULL, offset);
     }

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  \brief Release directory.
 *
 *  Equivalent to closedir (man 3 closedir).
 *
 *  \remarks Introduced in version 2.3.
 *
 *  \param ePath path to the file
 *  \param fi pointer to fuse file information
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_releasedir (const char *ePath, struct fuse_file_info *fi)
{
  soColorProbe (134, "07;31", "sofs_releasedir_bin (\"%s\", %p)\n", ePath, fi);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soClosedir (ePath);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  \brief Synchronize directory contents.
 *
 *  If the datasync parameter is non-zero, then only the user data should be flushed, not the meta data.
 *
 *  \remarks Introduced in version 2.3.
 *
 *  \param ePath path to the file
 *  \param isdatasync flag signaling if only the user data should be flushed
 *  \param fi pointer to fuse file information
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_fsyncdir (const char *ePath, int isdatasync, struct fuse_file_info *fi)
{
  soColorProbe (135, "07;31", "sofs_fsyncdir_bin (\"%s\", %d, %p)\n", ePath, isdatasync, fi);

  return soFsync (ePath);
}

/**
 *  \brief Create a symbolic link.
 *
 *  Similar to system call symlink (man 2 symlink).
 *
 *  \param effPath path to be stored in the symbolic link file
 *  \param ePath path to the symbolic link
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_symlink (const char *effPath, const char *ePath)
{
  soColorProbe (136, "07;31", "sofs_symlink_bin (\"%s\", \"%s\")\n", effPath, ePath);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soSymlink (effPath, ePath);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  \brief Read the target of a symbolic link.
 *
 *  Similar to system call readlink (man 2 readlink).
 *
 *  The buffer should be filled with a null terminated string. The buffer size argument includes the space
 *  for the terminating null character. If the linkname is too long to fit in the buffer, it should be
 *  truncated. The return value should be 0 for success.
 *
 *  \param ePath path to the symbolic link
 *  \param buf pointer to the buffer where data to be read is to be stored
 *  \param size buffer size in bytes
 *
 *  \return 0, on success, and a negative value, on error
 */

static int sofs_readlink (const char *ePath, char *buf, size_t size)
{
  soColorProbe (137, "07;31", "sofs_readlink_bin (\"%s\", %p, %"PRIu32")\n", ePath, buf, (uint32_t) size);

  int stat;

  if (pthread_mutex_lock (&accessCR) != 0)                           /* enter critical region */
     return -ENOLCK;

  stat = soReadlink (ePath, buf, (uint32_t) size);

  if (pthread_mutex_unlock (&accessCR) != 0)                         /* exit critical region */
     return -ENOLCK;

  return stat;
}

/**
 *  \brief Set extended attributes.
 *
 *  Equivalent to setxattr (man 2 setxattr).
 *
 *  \remarks UNIMPLEMENTED.
 */

static int sofs_setxattr (const char *ePath, const char *name, const char *value, size_t size, int flags)
{
  soColorProbe (138, "07;31", "sofs_setxattr_bin (\"%s\", \"%s\", %p, %"PRIu32", %d)\n", ePath, name, value,
                (uint32_t) size, flags);

  return -ENOSYS;
}

/**
 *  \brief Get extended attributes.
 *
 *  Equivalent to getxattr (man 2 getxattr).
 *
 *  \remarks UNIMPLEMENTED.
 */

static int sofs_getxattr (const char *ePath, const char *name, char *value, size_t size)
{
  soColorProbe (139, "07;31", "sofs_getxattr_bin (\"%s\", \"%s\", %p, %"PRIu32")\n", ePath, name, value, (uint32_t) size);

  return -ENOSYS;
}

/**
 *  \brief List extended attributes.
 *
 *  Equivalent to listxattr (man 2 listxattr).
 *
 *  \remarks UNIMPLEMENTED.
 */

static int sofs_listxattr (const char *ePath, char *list, size_t size)
{
  soColorProbe (140, "07;31", "sofs_listxattr_bin (\"%s\", \"%s\", %"PRIu32")\n", ePath, list, (uint32_t) size);

  return -ENOSYS;
}

/**
 *  \brief Remove extended attributes.
 *
 *  Equivalent to removexattr (man 2 removexattr).
 *
 *  \remarks UNIMPLEMENTED.
 */

int sofs_removexattr (const char *ePath, const char *name)
{
  soColorProbe (141, "07;31", "sofs_removexattr_bin (\"%s\", \"%s\")\n", ePath, name);

  return -ENOSYS;
}

/**
 *  \brief Get directory contents.
 *
 *  \remarks Deprecated, use readdir() instead.
 */

static int sofs_getdir (const char *ePath, fuse_dirh_t handle, fuse_dirfil_t filler)
{
  soColorProbe (142, "07;31", "sofs_getdir_bin (\"%s\", ...)\n", ePath);

  return -ENOSYS;
}
