/**
 *  \file sofs_ifuncs_1.h (interface file)
 *
 *  \brief Set of operations to manage the double-linked lists of free inodes and free data clusters: level 1 of the
 *         internal file system organization.
 *
 *         The aim is to provide an unique description of the functions that operate at this level.
 *
 *  The operations are:
 *      \li allocate a free inode
 *      \li free the referenced inode
 *      \li allocate a free data cluster
 *      \li free the referenced data cluster.
 *
 *  \author Artur Carneiro Pereira September 2008
 *  \author Miguel Oliveira e Silva September 2009
 *  \author António Rui Borges - September 2010 / September 2011, September 2014
 *
 *  \remarks In case an error occurs, all functions return a negative value which is the symmetric of the system error
 *           or the local error that better represents the error cause. Local errors are out of the range of the
 *           system errors.
 */


#ifndef SOFS_IFUNCS_1_H_
#define SOFS_IFUNCS_1_H_

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

extern int soAllocInode (uint32_t type, uint32_t* p_nInode);

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

extern int soFreeInode (uint32_t nInode);

/**
 *  \brief Allocate a free data cluster and associate it to an inode.
 *
 *  The inode is supposed to be associated to a file (a regular file, a directory or a symbolic link), but the only
 *  consistency check at this stage should be to check if the inode is not free.
 *
 *  The cluster is retrieved from the retrieval cache of free data cluster references. If the cache is empty, it has to
 *  be replenished before the retrieval may take place. If the data cluster is in the dirty state, it has to be cleaned
 *  first. The header fields of the allocated cluster should be all filled in: <tt>prev</tt> and <tt>next</tt> should be
 *  set to \c NULL_CLUSTER and <tt>stat</tt> to the given inode number.
 *
 *  \param nInode number of the inode the data cluster should be associated to
 *  \param p_nClust pointer to the location where the logical number of the allocated data cluster is to be stored
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, the <em>inode number</em> is out of range or the <em>pointer to the logical data cluster
 *                      number</em> is \c NULL
 *  \return -\c ENOSPC, if there are no free data clusters
 *  \return -\c EIUININVAL, if the inode in use is inconsistent
 *  \return -\c EFDININVAL, if the free inode in the dirty state is inconsistent
 *  \return -\c ELDCININVAL, if the list of data cluster references belonging to an inode is inconsistent
 *  \return -\c EDCINVAL, if the data cluster header is inconsistent
 *  \return -\c EDCNOTIL, if the referenced data cluster is not in the list of direct references
 *  \return -\c EWGINODENB, if the <em>inode number</em> in the data cluster <tt>status</tt> field is different from the
 *                          provided <em>inode number</em>
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soAllocDataCluster (uint32_t nInode, uint32_t *p_nClust);

/**
 *  \brief Free the referenced data cluster.
 *
 *  The cluster is inserted into the insertion cache of free data cluster references. If the cache is full, it has to be
 *  depleted before the insertion may take place. The data cluster should be put in the dirty state (the <tt>stat</tt>
 *  of the header should remain as it is), the other fields of the header, <tt>prev</tt> and <tt>next</tt>, should be
 *  put to NULL_CLUSTER. The only consistency check to carry out at this stage is to check if the data cluster was
 *  allocated.
 *
 *  Notice that the first data cluster, supposed to belong to the file system root directory, can never be freed.
 *
 *  \param nClust logical number of the data cluster
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, the <em>data cluster number</em> is out of range or the data cluster is not allocated
 *  \return -\c EDCNALINVAL, if the data cluster has not been previously allocated
 *  \return -\c EDCINVAL, if the data cluster header is inconsistent
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected at some internal storage lower level
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

extern int soFreeDataCluster (uint32_t nClust);

#endif /* SOFS_IFUNCS_1_H_ */
