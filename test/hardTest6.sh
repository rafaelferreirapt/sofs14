#!/bin/bash 
rm -rf hardTest
mkdir hardTest

DZONE_START=3
TOTAL_DC=4

echo "../run/showblock_sofs14 -s 0 myDisk" > hardTest/prof
../run/showblock_sofs14 -s 0 myDisk.prof >> hardTest/prof

for (( i = 1; i <= $DZONE_START-1; i++ )); do
	echo "../run/showblock_sofs14 -i $i myDisk" >> hardTest/prof
	../run/showblock_sofs14 -i $i myDisk.prof >> hardTest/prof
done

for (( i = 0; i <= $TOTAL_DC; i++ )); do
	value=$((4*$i+$DZONE_START))
	echo "../run/showblock_sofs14 -R $value myDisk" >> hardTest/prof
	../run/showblock_sofs14 -R $value myDisk.prof >> hardTest/prof
done

#ours

echo "../run/showblock_sofs14 -s 0 myDisk" > hardTest/ours
../run/showblock_sofs14 -s 0 myDisk.ours >> hardTest/ours

for (( i = 1; i <= $DZONE_START-1; i++ )); do
	echo "../run/showblock_sofs14 -i $i myDisk" >> hardTest/ours
	../run/showblock_sofs14 -i $i myDisk.ours >> hardTest/ours
done

for (( i = 0; i <= $TOTAL_DC; i++ )); do
	value=$((4*$i+$DZONE_START))
	echo "../run/showblock_sofs14 -R $value myDisk" >> hardTest/ours
	../run/showblock_sofs14 -R $value myDisk.ours >> hardTest/ours
done

