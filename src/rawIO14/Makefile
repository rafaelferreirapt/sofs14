CC = gcc
CFLAGS = -Wall -I "../debugging"

all:			librawIO14

librawIO14:		sofs_rawdisk.o
			ar -r librawIO14.a $^
			cp librawIO14.a ../../lib
			rm -f $^ librawIO14.a

clean:
			rm -f ../../lib/librawIO14.a
