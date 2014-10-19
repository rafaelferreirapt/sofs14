#!/bin/bash

# This test vector deals mainly with operations get a directory entry by name and add / remove directory
# entries.
# It defines a storage device with 98 blocks and formats it with 72 inodes.
# It starts by allocating thirty two inodes, associated to regular files and directories, and organize
# them as entries in the root directory. It proceeds by removing all of them. Finally, it allocates two
# additional inodes, associated to regular files, and organize them also as entries of the root
# directory.
# The major aim is to check the growth of the root directory and the use of freed directory entries.
# The showblock_sofs14 application should be used in the end to check metadata.

RUNDIR=../run

case "$1" in
    "-bin") mkfs=mkfs_sofs14_bin;;
    *) mkfs=mkfs_sofs14;;
esac

${RUNDIR}/createEmptyFile myDisk 98
${RUNDIR}/${mkfs} -n SOFS14 -i 72 -z myDisk
${RUNDIR}/testifuncs14 -b -l 300,700 -L testVector13.rst myDisk <testVector13.cmd
