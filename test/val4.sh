RUNDIR=../run

case "$1" in
    "-bin") mkfs=mkfs_sofs14_bin;;
    *) mkfs=mkfs_sofs14;;
esac

#!/bin/bash

# This test vector checks if a relatively complex directory hierarchy can be built in the file system.
# Basic system calls involved: readdir, mknode, read, write, mkdir and link.

echo -e '\n**** Creating the storage device.****\n'
${RUNDIR}/createEmptyFile myDisk 300
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
ln mnt/ex/ex10.sh mnt/new/newAgain/sameAsEx10.sh
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
echo -e '\n**** Displaying the file contents through a hard link which was purposefully created.****\n'
cat mnt/new/newAgain/sameAsEx10.sh
echo -e '\n**** Getting the hard link attributes.****\n'
stat mnt/new/newAgain/sameAsEx10.sh
echo -e '\n**** Unmounting the storage device.****\n'
sleep 1
fusermount -u mnt
