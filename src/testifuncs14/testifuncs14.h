/**
 *  \file testifuncs14.h (interface file)
 *
 *  \brief The SOFS14 internal testing tool.
 *
 *  It provides a simple method to test separately the file system internal operations.
 *
 *  Level 1 - Management of the double-linked lists of free inodes and free data clusters:
 *     \li allocate a free inode
 *     \li free the referenced inode
 *     \li allocate a free data cluster
 *     \li free the referenced data cluster.
 *
 *  Level 2 - Management of inodes:
 *     \li read specific inode data from the table of inodes
 *     \li write specific inode data to the table of inodes
 *     \li clean an inode
 *     \li check the inode access permissions against a given operation.
 *
 *  Level 3 - Management of data clusters:
 *     \li read a specific data cluster
 *     \li write to a specific data cluster
 *     \li handle a file data cluster
 *     \li free and clean all data clusters from the list of references starting at a given point
 *     \li clean a data cluster from the inode describing a file which was previously deleted.
 *
 *  Level 4 - Management of directories and directory entries:
 *      \li get an entry by path
 *      \li get an entry by name
 *      \li add an new entry / attach a directory entry to a directory
 *      \li remove an entry / detach a directory entry from a directory
 *      \li rename an entry of a directory
 *      \li check a directory status of emptiness.
 *
 *  SINOPSIS:
 *  <P><PRE>                testifuncs14 [OPTIONS] supp-file

                  OPTIONS:
                   -b       --- set batch mode (default: not batch)
                   -l depth --- set log depth (default: 0,0)
                   -L file  --- log file (default: stdout)
                   -h       --- print this help.</PRE>
 *
 *  \author Artur Carneiro Pereira - October 2005
 *  \author Miguel Oliveira e Silva - September 2009
 *  \author António Rui Borges - October 2010 / September 2011, September 2014
 */
