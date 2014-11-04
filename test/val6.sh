RUNDIR=../run

case "$1" in
    "-bin") mkfs=mkfs_sofs14_bin;;
    *) mkfs=mkfs_sofs14;;
esac

#!/bin/bash

# This test vector checks regular file truncation.
# Basic system calls involved: readdir, mknode, read, write, mkdir, link and truncate.

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
ln mnt/ex/ex10.sh mnt/new/newAgain/sameAsEx10.sh
sleep 1
echo -e '\n**** Listing the directory mnt/ex.****\n'
ls -la mnt/ex
echo -e '\n**** Displaying the file mnt/ex/ex4.sh contents.****\n'
cat mnt/ex/ex4.sh
echo -e '\n**** Getting the file attributes.****\n'
stat mnt/ex/ex4.sh
echo -e '\n**** Truncating it to 400 bytes.****\n'
truncate -s 400 mnt/ex/ex4.sh
echo -e '\n**** Displaying the file mnt/ex/ex4.sh contents after truncation.****\n'
cat mnt/ex/ex4.sh
echo -e '\n\n**** Getting the file attributes.****\n'
stat mnt/ex/ex4.sh
echo -e '\n**** Displaying the file mnt/ex/ex3.sh contents.****\n'
cat mnt/ex/ex3.sh
echo -e '\n**** Getting the file attributes.****\n'
stat mnt/ex/ex3.sh
echo -e '\n**** Truncating it to 4000 bytes.****\n'
truncate -s 4000 mnt/ex/ex3.sh
echo -e '\n**** Displaying the file mnt/ex/ex3.sh contents after truncation.****\n'
cat mnt/ex/ex3.sh
echo -e '\n**** Getting the file attributes.****\n'
stat mnt/ex/ex3.sh
echo -e '\n**** Unmounting the storage device.****\n'
sleep 1
fusermount -u mnt
