#!/bin/bash

# This test vector deals with the operations alloc inode and alloc / free data clusters.
# It defines a storage device with 100 blocks.
# It starts by allocating some inodes. Then, it allocates some data clusters and tests different error conditions. Finally, it frees the data
# clusters and also tests some error conditions, in particular that data cluster #0 can not be freed.
# The showblock_sofs14 application should be used in the end to check metadata.

RUNDIR=../run

case "$1" in
    "-bin") mkfs=mkfs_sofs14_bin;;
    *) mkfs=mkfs_sofs14;;
esac

${RUNDIR}/createEmptyFile myDisk 100
${RUNDIR}/${mkfs} -n SOFS14 -i 8 -z myDisk
${RUNDIR}/testifuncs14 -b -l 600,700 -L testVector3.rst myDisk <testVector3.cmd
