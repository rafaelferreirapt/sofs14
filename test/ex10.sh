#!/bin/bash

# This test vector deals mainly with operations write data clusters and handle file clusters with option FREE_CLEAN.
# It defines a storage device with 1000 blocks and formats it with 247 data clusters.
# It starts by allocating an inode, then it proceeds by allocating 60 data clusters in all the reference areas (direct, single indirect and
# double indirect). This means that in fact 70 data clusters are allocated.
# Then all the data clusters are freed and cleaned.
# The showblock_sofs14 application should be used in the end to check metadata.

RUNDIR=../run

case "$1" in
    "-bin") mkfs=mkfs_sofs14_bin;;
    *) mkfs=mkfs_sofs14;;
esac

${RUNDIR}/createEmptyFile myDisk 1000
${RUNDIR}/${mkfs} -n SOFS14 -i 80 -z myDisk
${RUNDIR}/testifuncs14 -b -l 400,700 -L testVector10.rst myDisk <testVector10.cmd
