.PHONY: all doc clean

all:
	make -C src all

doc:
	cd ./doc; doxygen

clean:
	make -C src clean
	cd ./doc; rm -rf html

