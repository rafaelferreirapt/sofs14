#!/bin/bash

# This test vector checks if FUSE can mount the storage device in an empty file system scenario.
# It only checks if the formatting tool is operational.
# Basic system calls involved: readdir.

RUNDIR=../run

case "$1" in
    "-bin") mkfs=mkfs_sofs14_bin;;
    *) mkfs=mkfs_sofs14;;
esac

echo -e '\n**** Creating the storage device.****\n'
${RUNDIR}/createEmptyFile myDisk 100
echo -e '\n**** Converting the storage device into a SOFS14 file system.****\n'
${RUNDIR}/${mkfs} -i 56 -z myDisk
echo -e '\n**** Mounting the storage device as a SOFS14 file system.****\n'
${RUNDIR}/mount_sofs14 myDisk mnt
echo -e '\n**** Listing the root directory.****\n'
ls -la mnt
echo -e '\n**** Getting the file system attributes.****\n'
stat -f mnt/.
echo -e '\n**** Getting the root directory attributes.****\n'
stat mnt/.
echo -e '\n**** Unmounting the storage device.****\n'
sleep 1
fusermount -u mnt
