#!/bin/bash

# This test vector deals mainly with operations alloc / free data clusters through the operation handle file cluster which is always used.
# It defines a storage device with 100 blocks and formats it with 24 data clusters.
# It starts by allocating an inode, then it proceeds by allocating 13 data clusters in all the reference areas (direct, single indirect and
# double indirect). This means that in fact 20 data clusters are allocated.
# Then all data clusters are freed in reverse order and the inode is also freed, leaving it in the dirty state. This means that only 13 data
# clusters are in fact freed.
# The showblock_sofs14 application should be used in the end to check metadata.

RUNDIR=../run

case "$1" in
    "-bin") mkfs=mkfs_sofs14_bin;;
    *) mkfs=mkfs_sofs14;;
esac

${RUNDIR}/createEmptyFile myDisk 100
${RUNDIR}/${mkfs} -n SOFS14 -i 16 -z myDisk
${RUNDIR}/testifuncs14 -b -l 400,700 -L testVector7.rst myDisk <testVector7.cmd
