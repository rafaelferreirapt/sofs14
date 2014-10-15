#!/bin/bash

# This test vector deals mainly with operations alloc / free / clean data clusters through the operation handle file cluster which is always used.
# It defines a storage device with 100 blocks and formats it with 24 data clusters.
# It starts by allocating an inode, then it proceeds by allocating 13 data clusters in all the reference areas (direct, single indirect and
# double indirect). This means that in fact 20 data clusters are allocated.
# Then all data clusters are freed and the inode is also freed, leaving it in the dirty state. This means that only 13 data clusters are in
# fact freed.
# Afterwards, another inode is allocated and 6 data clusters are also allocated (2 in the direct area and 4 in the single indirect area),
# which means that all but 11 data clusters are presently allocated. (Why?)
# The aim is to check if the operations are performed correctly even when the allocated data clusters are in the dirty state.
# The showblock_sofs14 application should be used in the end to check metadata.

RUNDIR=../run

case "$1" in
    "-bin") mkfs=mkfs_sofs14_bin;;
    *) mkfs=mkfs_sofs14;;
esac

${RUNDIR}/createEmptyFile myDisk 100
${RUNDIR}/${mkfs} -n SOFS14 -i 24 -z myDisk
${RUNDIR}/testifuncs14 -b -l 400,700 -L testVector8.rst myDisk <testVector8.cmd
