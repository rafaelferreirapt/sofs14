RUNDIR=../run

case "$1" in
    "-bin") mkfs=mkfs_sofs14_bin;;
    *) mkfs=mkfs_sofs14;;
esac

#!/bin/bash

# This test vector checks if a relatively complex directory hierarchy can be built in the file system and later deleted.
# Basic system calls involved: readdir, mknode, read, write, mkdir, link, rmdir and unlink.

echo -e '\n**** Creating the storage device.****\n'
${RUNDIR}/createEmptyFile myDisk 200
echo -e '\n**** Converting the storage device into a SOFS14 file system.****\n'
${RUNDIR}/${mkfs} -i 56 -z myDisk
echo -e '\n**** Mounting the storage device as a SOFS14 file system.****\n'
${RUNDIR}/mount_sofs14 myDisk mnt
echo -e '\n**** Creating the directory hierarchy.****\n'
mkdir mnt/ex
mkdir mnt/testVec
mkdir mnt/new
mkdir mnt/new/newAgain
cp ex*.sh mnt/ex
cp testVector*.cmd mnt/testVec
ln mnt/ex/ex10.sh mnt/ex/sameAsEx10.sh
ln mnt/ex/ex10.sh mnt/new/newAgain/sameAsEx10.sh
sleep 1
echo -e '\n**** Listing all the directories.****\n'
ls -la mnt
echo -e '\n********\n'
ls -la mnt/ex
echo -e '\n********\n'
ls -la mnt/testVec
echo -e '\n********\n'
ls -la mnt/new
echo -e '\n********\n'
ls -la mnt/new/newAgain
echo -e '\n**** Try to the delete directory mnt/new/newAgain.****\n'
echo -e '**** This should fail because the directory is not empty!****\n'
rmdir  mnt/new/newAgain
echo -e '\n**** Delete directory mnt/new/newAgain now in a orderly faction.****\n'
rm -f mnt/new/newAgain/*
rmdir mnt/new/newAgain
sleep 1
echo -e '\n**** Listing the directories mnt/ex and mnt/new.****\n'
echo -e '**** Note that the file mnt/ex/ex10.sh has now only two links!****\n'
ls -la mnt/ex
echo -e '\n********\n'
ls -la mnt/new
echo -e '\n**** Delete the remaining contents of the root directory.****\n'
rm -rf mnt/*
echo -e '\n**** Listing the root directory.****\n'
ls -la mnt
echo -e '\n**** Unmounting the storage device.****\n'
sleep 1
fusermount -u mnt
