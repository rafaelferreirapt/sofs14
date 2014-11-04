RUNDIR=../run

case "$1" in
    "-bin") mkfs=mkfs_sofs14_bin;;
    *) mkfs=mkfs_sofs14;;
esac

#!/bin/bash

# This test vector checks if a large pdf file can be copied to the root directory.
# Basic system calls involved: readdir, mknode, read and write.

echo -e '\n**** Creating the storage device.****\n'
${RUNDIR}/createEmptyFile myDisk 1000
echo -e '\n**** Converting the storage device into a SOFS14 file system.****\n'
${RUNDIR}/${mkfs} -i 56 -z myDisk
echo -e '\n**** Mounting the storage device as a SOFS14 file system.****\n'
${RUNDIR}/mount_sofs14 myDisk mnt
echo -e '\n**** Copying the text file.****\n'
cp "../doc/SOFS14.pdf" mnt
echo -e '\n**** Listing the root directory.****\n'
ls -la mnt
echo -e '\n**** Checking if the file was copied correctly.****\n'
diff "../doc/SOFS14.pdf" "mnt/SOFS14.pdf"
echo -e '\n**** Getting the file attributes.****\n'
stat mnt/"SOFS14.pdf"
echo -e '\n**** Unmounting the storage device.****\n'
sleep 1
fusermount -u mnt
