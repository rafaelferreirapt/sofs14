#!/bin/bash

# This test vector deals mainly with the operation get a directory entry by path.
# It defines a storage device with 98 blocks and formats it with 72 inodes.
# It starts by allocating seven inodes, associated to regular files and directories, and organize them
# in a hierarchical faction. Then it proceeds by trying to find different directory entries through the
# use of different paths.
# The showblock_sofs14 application should be used in the end to check metadata.

RUNDIR=../run

case "$1" in
    "-bin") mkfs=mkfs_sofs14_bin;;
    *) mkfs=mkfs_sofs14;;
esac

${RUNDIR}/createEmptyFile myDisk 98
${RUNDIR}/${mkfs} -n SOFS14 -i 72 -z myDisk
${RUNDIR}/testifuncs14 -b -l 300,700 -L testVector14.rst myDisk <testVector14.cmd
