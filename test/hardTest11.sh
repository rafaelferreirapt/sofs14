#!/bin/bash 
rm -rf hardTest
mkdir hardTest

echo "../run/showblock_sofs14 -s 0 myDisk" > hardTest/prof11
../run/showblock_sofs14 -s 0 myDisk.prof >> hardTest/prof11

for (( i = 1; i <= 1; i++ )); do
	echo "../run/showblock_sofs14 -i $i myDisk" >> hardTest/prof11
	../run/showblock_sofs14 -i $i myDisk.prof >> hardTest/prof11
done

for (( i = 0; i <= 249; i++ )); do
	value=$((4*$i+2))
	echo "../run/showblock_sofs14 -R $value myDisk" >> hardTest/prof11
	../run/showblock_sofs14 -R $value myDisk.prof >> hardTest/prof11
done

#ours

echo "../run/showblock_sofs14 -s 0 myDisk" > hardTest/ours11
../run/showblock_sofs14 -s 0 myDisk.ours >> hardTest/ours11

for (( i = 1; i <= 1; i++ )); do
	echo "../run/showblock_sofs14 -i $i myDisk" >> hardTest/ours11
	../run/showblock_sofs14 -i $i myDisk.ours >> hardTest/ours11
done

for (( i = 0; i <= 249; i++ )); do
	value=$((4*$i+2))
	echo "../run/showblock_sofs14 -R $value myDisk" >> hardTest/ours11
	../run/showblock_sofs14 -R $value myDisk.ours >> hardTest/ours11
done

