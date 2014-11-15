#!/bin/bash 
rm -rf hardTest
mkdir hardTest

echo "../run/showblock_sofs14 -s 0 myDisk" > hardTest/prof10
../run/showblock_sofs14 -s 0 myDisk.prof >> hardTest/prof10

for (( i = 1; i <= 11; i++ )); do
	echo "../run/showblock_sofs14 -i $i myDisk" >> hardTest/prof10
	../run/showblock_sofs14 -i $i myDisk.prof >> hardTest/prof10
done

for (( i = 0; i <= 247; i++ )); do
	value=$((4*$i+12))
	echo "../run/showblock_sofs14 -R $value myDisk" >> hardTest/prof10
	../run/showblock_sofs14 -R $value myDisk.prof >> hardTest/prof10
done

#ours

echo "../run/showblock_sofs14 -s 0 myDisk" > hardTest/ours10
../run/showblock_sofs14 -s 0 myDisk.ours >> hardTest/ours10

for (( i = 1; i <= 11; i++ )); do
	echo "../run/showblock_sofs14 -i $i myDisk" >> hardTest/ours10
	../run/showblock_sofs14 -i $i myDisk.ours >> hardTest/ours10
done

for (( i = 0; i <= 247; i++ )); do
	value=$((4*$i+12))
	echo "../run/showblock_sofs14 -R $value myDisk" >> hardTest/ours10
	../run/showblock_sofs14 -R $value myDisk.ours >> hardTest/ours10
done

