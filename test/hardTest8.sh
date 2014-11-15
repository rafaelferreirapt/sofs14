#!/bin/bash 
rm -rf hardTest
mkdir hardTest

echo "../run/showblock_sofs14 -s 0 myDisk" > hardTest/prof8
../run/showblock_sofs14 -s 0 myDisk.prof >> hardTest/prof8

echo "../run/showblock_sofs14 -i 1 myDisk" >> hardTest/prof8
../run/showblock_sofs14 -i 1 myDisk.prof >> hardTest/prof8

echo "../run/showblock_sofs14 -i 2 myDisk" >> hardTest/prof8
../run/showblock_sofs14 -i 2 myDisk.prof >> hardTest/prof8


for (( i = 0; i <= 24; i++ )); do
	value=$((4*$i+4))
	echo "../run/showblock_sofs14 -R $value myDisk" >> hardTest/prof8
	../run/showblock_sofs14 -R $value myDisk.prof >> hardTest/prof8
done

#ours

echo "../run/showblock_sofs14 -s 0 myDisk" > hardTest/ours8
../run/showblock_sofs14 -s 0 myDisk.ours >> hardTest/ours8

echo "../run/showblock_sofs14 -i 1 myDisk" >> hardTest/ours8
../run/showblock_sofs14 -i 1 myDisk.ours >> hardTest/ours8

echo "../run/showblock_sofs14 -i 2 myDisk" >> hardTest/ours8
../run/showblock_sofs14 -i 2 myDisk.ours >> hardTest/ours8


for (( i = 0; i <= 24; i++ )); do
	value=$((4*$i+4))
	echo "../run/showblock_sofs14 -R $value myDisk" >> hardTest/ours8
	../run/showblock_sofs14 -R $value myDisk.ours >> hardTest/ours8
done

