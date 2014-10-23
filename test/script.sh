#!/bin/bash 
#criar as pastas para organizar os outputs e os diffs
mkdir prof
mkdir ours

if [[ "$1" == "1" ]]; then
	FROM=1
	TO=5
elif [[ "$1" == "2" ]]; then
	FROM=6
	TO=6
elif [[ "$1" == "3" ]]; then
	FROM=7
	TO=11
elif [[ "$1" == "4" ]]; then
	FROM=12
	TO=16
else
	FROM=1
	TO=16
fi

#com o makefile do prof (Makefile)
cd ../src/sofs14
mv Makefile.prof Makefile
cd ../../
make
cd src/sofs14
mv Makefile Makefile.prof
cd ../../test
for (( i = ${FROM}; i <= ${TO}; i++ )); do
	./ex$i.sh
done
mv *.rst prof
cd ../src/sofs14

#com o makefile do prof (Makefile)
mv Makefile.ours Makefile
cd ../../
make
cd src/sofs14
mv Makefile Makefile.ours
cd ../../test
for (( i = ${FROM}; i <= ${TO}; i++ )); do
	./ex$i.sh
done
mv *.rst ours
cd ../src/sofs14
cp Makefile.prof Makefile

#fazer os diffs
cd ../../test
for (( i = ${FROM}; i <= ${TO}; i++ )); do
	diff ours/testVector$i.rst prof/testVector$i.rst >> diff/result$i.html
done


