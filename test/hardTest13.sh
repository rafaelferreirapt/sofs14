#!/bin/bash 
rm -rf hardTest
mkdir hardTest

echo "../run/showblock_sofs14 -s 0 myDisk" > hardTest/prof13
../run/showblock_sofs14 -s 0 myDisk.prof >> hardTest/prof13

for (( i = 1; i <= 9; i++ )); do
	echo "../run/showblock_sofs14 -i $i myDisk" >> hardTest/prof13
	../run/showblock_sofs14 -i $i myDisk.prof >> hardTest/prof13
done

for (( i = 0; i <= 22; i++ )); do
	value=$((4*$i+10))
	echo "../run/showblock_sofs14 -R $value myDisk" >> hardTest/prof13
	../run/showblock_sofs14 -R $value myDisk.prof >> hardTest/prof13
done

#ours

echo "../run/showblock_sofs14 -s 0 myDisk" > hardTest/ours13
../run/showblock_sofs14 -s 0 myDisk.ours >> hardTest/ours13

for (( i = 1; i <= 9; i++ )); do
	echo "../run/showblock_sofs14 -i $i myDisk" >> hardTest/ours13
	../run/showblock_sofs14 -i $i myDisk.ours >> hardTest/ours13
done

for (( i = 0; i <= 22; i++ )); do
	value=$((4*$i+10))
	echo "../run/showblock_sofs14 -R $value myDisk" >> hardTest/ours13
	../run/showblock_sofs14 -R $value myDisk.ours >> hardTest/ours13
done

