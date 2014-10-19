#!/bin/bash

# This test vector deals mainly with operations get a directory entry by name, add / remove directory
# entries, rename directory entries and check a directory status of emptiness.
# It defines a storage device with 98 blocks and formats it with 72 inodes.
# It starts by allocating seven inodes, associated to regular files and directories, and organize them
# in a hierarchical faction. Then it proceeds by renaming some of the entries and ends by removing all
# of them.
# The showblock_sofs14 application should be used in the end to check metadata.

RUNDIR=../run

case "$1" in
    "-bin") mkfs=mkfs_sofs14_bin;;
    *) mkfs=mkfs_sofs14;;
esac

${RUNDIR}/createEmptyFile myDisk 98
${RUNDIR}/${mkfs} -n SOFS14 -i 72 -z myDisk
${RUNDIR}/testifuncs14 -b -l 300,700 -L testVector12.rst myDisk <testVector12.cmd
