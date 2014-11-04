#!/bin/bash 
#criar as pastas para organizar os outputs e os diffs
rm -rf prof
mkdir prof
rm -rf profRaw
mkdir profRaw
rm -rf ours
mkdir ours
rm -rf oursRaw
mkdir oursRaw

filter_base()
{
    sed -r 's_.[[].*[[]0m_+++_' \
        | sed -r 's_0x[0-9a-fA-Z]{6,12}_..._g' \
        | sed -r 's_atime = .*, mtime = .*_atime ..., mtime ..._'
}
filter_bin()
{
    sed -r 's/_bin//'
}
val(){
	#com o makefile do prof (Makefile)
	cd ../src/sofs14
	mv Makefile.prof Makefile
	cd ../../
	make
	cd src/sofs14
	mv Makefile Makefile.prof
	cd ../../test
	for (( i = 1; i <= 7; i++ )); do
		./val$i.sh > valTest$i.rst
	done
	mv *.rst profRaw
	cd ../src/sofs14

	#com o makefile do prof (Makefile)
	mv Makefile.ours Makefile
	cd ../../
	make
	cd src/sofs14
	mv Makefile Makefile.ours
	cd ../../test
	for (( i = 1; i <= 7; i++ )); do
		./val$i.sh > valTest$i.rst
	done
	mv *.rst oursRaw
	cd ../src/sofs14
	cp Makefile.prof Makefile
}

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
elif [[ "$1" == "5" ]]; then
	val
	exit
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
	cat testVector$i.rst | filter_base | filter_bin > testVector$i.cleaned.rst
done
mv *.cleaned.rst prof
mv *.rst profRaw
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
	cat testVector$i.rst | filter_base | filter_bin > testVector$i.cleaned.rst
done
mv *.cleaned.rst ours
mv *.rst oursRaw
cd ../src/sofs14
cp Makefile.prof Makefile
