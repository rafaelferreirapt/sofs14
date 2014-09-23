CC = gcc
CFLAGS = -Wall -I "../debugging" -I "../rawIO14" -I "../sofs14"
LFLAGS = -L "../../lib"

all32:			mkfs_sofs14_32

mkfs_sofs14_32:		mkfs_sofs14.o
			$(CC) $(LFLAGS) -o mkfs_sofs14 $^ -lsofs14bin_32 -lsofs14 -lrawIO14bin_32 -lrawIO14 -ldebugging
			cp mkfs_sofs14 ../../run
			rm -f $^ mkfs_sofs14

all64:			mkfs_sofs14_64

mkfs_sofs14_64:		mkfs_sofs14.o
			$(CC) $(LFLAGS) -o mkfs_sofs14 $^ -lsofs14bin_64 -lsofs14 -lrawIO14bin_64 -lrawIO14 -ldebugging
			cp mkfs_sofs14 ../../run
			rm -f $^ mkfs_sofs14

clean:
			rm -f ../../run/mkfs_sofs14
