#!/bin/bash 
#criar as pastas para organizar os outputs e os diffs
mkdir prof
mkdir diff
mkdir ours

#if [[ $1 == 1 ]]; then
#	FROM = 1
#	TO = 5
#elif [[ $1 == 2 ]]; then
#	FROM = 6
#	TO = 6
#elif [[ $1 == 3 ]]; then
#	FROM = 7
#	TO = 11
#elif [[ $1 == 4 ]]; then
#	FROM = 12
#	TO = 16
#else
#	FROM = 1
#	TO = 16
#fi
NUMBER_TEST=16

#com o makefile do prof (Makefile)
cd ../src/sofs14
#mv Makefile.prof Makefile
cd ../../
make
cd test
for (( i = 1; i <= ${NUMBER_TEST}; i++ )); do
	./ex$i.sh
done
mv *.rst prof
cd ../src/sofs14
mv Makefile Makefile.prof

#com o makefile do prof (Makefile)
mv Makefile.ours Makefile
cd ../../
make
cd test
for (( i = 1; i <= ${NUMBER_TEST}; i++ )); do
	./ex$i.sh
done
mv *.rst ours
cd ../src/sofs14
mv Makefile Makefile.ours
cp Makefile.prof Makefile

#fazer os diffs
cd ../../test
for (( i = 1; i <= ${NUMBER_TEST}; i++ )); do
	diff ours/testVector$i.rst prof/testVector$i.rst >> diff/result$i.html
done


