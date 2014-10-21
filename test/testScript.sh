#criar as pastas para organizar os outputs e os diffs
mkdir prof
mkdir diff
mkdir ours

#com o makefile do prof (Makefile)
cd ../src/sofs14
mv Makefile.prof Makefile
cd ../../
make
cd test
for (( i = 1; i <= 11; i++ )); do
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
for (( i = 1; i <= 11; i++ )); do
	./ex$i.sh
done
mv *.rst ours
cd ../src/sofs14
mv Makefile Makefile.ours
cp Makefile.prof Makefile

#fazer os diffs
cd ../../test
for (( i = 1; i <= 11; i++ )); do
	diff ours/testVector$i.rst prof/testVector$i.rst >> diff/result$i.html
done


