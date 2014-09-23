/**
 *  \file sofs_direntry.h (interface file)
 *
 *  \brief Definition of the directory entry data type.
 *
 *  It specifies the file system metadata which describes how directories are organized as arrays of these elements.
 *
 *  \author Artur Carneiro Pereira - September 2008
 *  \author Miguel Oliveira e Silva - September 2009
 *  \author António Rui Borges - August 2011
 */

#ifndef SOFS_DIRENTRY_H_
#define SOFS_DIRENTRY_H_

#include <stdint.h>

#include "sofs_const.h"

/** \brief maximum length of a file name (in characters) */
#define MAX_NAME 59

/** \brief maximum length of a file path within the file system (in characters) */
#define MAX_PATH 254

/**
 *  \brief Definition of the directory entry data type.
 *
 *  It is divided in:
 *     \li <em>name of the file</em> - as it is generically referred to
 *     \li <em>number of the inode</em> - where the file attributes are stored.
 */

typedef struct soDirEntry
{
   /** \brief the name of a file (whether a regular file, a directory or a symbolic link):
    *  it must be a NUL-terminated string
    */
    unsigned char name[MAX_NAME+1];
   /** \brief the associated inode number */
    uint32_t nInode;
} SODirEntry;

#endif /* SOFS_DIRENTRY_H_ */
