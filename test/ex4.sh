#!/bin/bash

# This test vector deals with the operations alloc inode and alloc / free data clusters.
# It defines a storage device with 19 blocks and formats it with an inode table of 16 inodes.
# It starts by allocating inodes. Then, it allocates all the data clusters. Finally, it frees all the data
# clusters in reverse order and allocates another.
# The showblock_sofs14 application should be used in the end to check metadata.

RUNDIR=../run

case "$1" in
    "-bin") mkfs=mkfs_sofs14_bin;;
    *) mkfs=mkfs_sofs14;;
esac

${RUNDIR}/createEmptyFile myDisk 19
${RUNDIR}/${mkfs} -n SOFS14 -i 16 -z myDisk
${RUNDIR}/testifuncs14 -b -l 400,700 -L testVector4.rst myDisk <testVector4.cmd
