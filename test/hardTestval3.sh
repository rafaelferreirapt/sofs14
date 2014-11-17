#!/bin/bash 
rm -rf hardTest
mkdir hardTest

echo "../run/showblock_sofs14 -s 0 myDisk" > hardTest/profVal3
../run/showblock_sofs14 -s 0 myDisk.prof >> hardTest/profVal3

for (( i = 1; i <= 7; i++ )); do
	echo "../run/showblock_sofs14 -i $i myDisk" >> hardTest/profVal3
	../run/showblock_sofs14 -i $i myDisk.prof >> hardTest/profVal3
done

for (( i = 0; i <= 248; i++ )); do
	value=$((4*$i+8))
	echo "../run/showblock_sofs14 -R $value myDisk" >> hardTest/profVal3
	../run/showblock_sofs14 -R $value myDisk.prof >> hardTest/profVal3
done

#ours

echo "../run/showblock_sofs14 -s 0 myDisk" > hardTest/oursVal3
../run/showblock_sofs14 -s 0 myDisk.ours >> hardTest/oursVal3

for (( i = 1; i <= 7; i++ )); do
	echo "../run/showblock_sofs14 -i $i myDisk" >> hardTest/oursVal3
	../run/showblock_sofs14 -i $i myDisk.ours >> hardTest/oursVal3
done

for (( i = 0; i <= 248; i++ )); do
	value=$((4*$i+8))
	echo "../run/showblock_sofs14 -R $value myDisk" >> hardTest/oursVal3
	../run/showblock_sofs14 -R $value myDisk.ours >> hardTest/oursVal3
done

