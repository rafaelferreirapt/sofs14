/**
 *  \file soFreeDataCluster.c (implementation file)
 *
 *  \author
 */

#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
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

/* Allusion to internal function */

int soDeplete (SOSuperBlock *p_sb);

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

int soFreeDataCluster (uint32_t nClust)
{
  soColorProbe (614, "07;33", "soFreeDataCluster (%"PRIu32")\n", nClust);

  /* insert your code here */

  return 0;
}

/**
 *  \brief Deplete the insertion cache of free data cluster references.
 *
 *  \param p_sb pointer to a buffer where the superblock data is stored
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c ELIBBAD, if some kind of inconsistency was detected
 *  \return -\c EBADF, if the device is not already opened
 *  \return -\c EIO, if it fails reading or writing
 *  \return -<em>other specific error</em> issued by \e lseek system call
 */

int soDeplete (SOSuperBlock *p_sb)
{

  /* insert your code here */

  return 0;
}
