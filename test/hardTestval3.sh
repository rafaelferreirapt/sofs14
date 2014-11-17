#!/bin/bash 
rm -rf hardTest
mkdir hardTest

RUNDIR=../run

case "$1" in
"-bin") mkfs=mkfs_sofs14_bin;;
*) mkfs=mkfs_sofs14;;
esac

echo -e '\n**** Creating the storage device.****\n'
${RUNDIR}/createEmptyFile myDisk.prof 1000
echo -e '\n**** Converting the storage device into a SOFS14 file system.****\n'
${RUNDIR}/${mkfs} -i 56 -z myDisk.prof
echo -e '\n**** Mounting the storage device as a SOFS14 file system.****\n'
${RUNDIR}/mount_sofs14 myDisk.prof mnt
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

for (( i = 0; i <= 61; i++ )); do
	value=$((4*$i+4))
	echo "../run/showblock_sofs14 -D $value myDisk" >> hardTest/prof8
	../run/showblock_sofs14 -D $value myDisk.prof >> hardTest/prof8
done

#ours

echo -e '\n**** Creating the storage device.****\n'
${RUNDIR}/createEmptyFile myDisk.ours 1000
echo -e '\n**** Converting the storage device into a SOFS14 file system.****\n'
${RUNDIR}/${mkfs} -i 56 -z myDisk.ours
echo -e '\n**** Mounting the storage device as a SOFS14 file system.****\n'
${RUNDIR}/mount_sofs14 myDisk.ours mnt
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

for (( i = 0; i <= 61; i++ )); do
value=$((4*$i+4))
echo "../run/showblock_sofs14 -D $value myDisk" >> hardTest/oursval3
../run/showblock_sofs14 -D $value myDisk.ours >> hardTest/oursval3
done
