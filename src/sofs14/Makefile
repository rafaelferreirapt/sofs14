CC = gcc
CFLAGS = -Wall -I "../debugging" -I "../rawIO14" -I.

LIB_NAME = sofs14

TARGET_LIB = lib$(LIB_NAME).a

OBJS  = sofs_basicoper.o 
OBJS += $(IFUNCS1) 
OBJS += $(IFUNCS2) 
OBJS += $(IFUNCS3) 
OBJS += $(IFUNCS4) 

#IFUNCS1  = sofs_ifuncs_1/soAllocInode.o
#IFUNCS1 += sofs_ifuncs_1/soFreeInode.o
#IFUNCS1 += sofs_ifuncs_1/soAllocDataCluster.o 
#IFUNCS1 += sofs_ifuncs_1/soFreeDataCluster.o

IFUNCS2  = sofs_ifuncs_2/soReadInode.o
#IFUNCS2 += sofs_ifuncs_2/soWriteInode.o
#IFUNCS2 += sofs_ifuncs_2/soCleanInode.o 
IFUNCS2 += sofs_ifuncs_2/soAccessGranted.o

IFUNCS3  = sofs_ifuncs_3/soReadFileCluster.o
#IFUNCS3 += sofs_ifuncs_3/soWriteFileCluster.o
#IFUNCS3 += sofs_ifuncs_3/soHandleFileCluster.o 
#IFUNCS3 += sofs_ifuncs_3/soHandleFileClusters.o
#IFUNCS3 += sofs_ifuncs_3/soCleanDataCluster.o

IFUNCS4  = sofs_ifuncs_4/soGetDirEntryByPath.o
IFUNCS4 += sofs_ifuncs_4/soGetDirEntryByName.o
#IFUNCS4 += sofs_ifuncs_4/soAddAttDirEntry.o 
#IFUNCS4 += sofs_ifuncs_4/soRemDetachDirEntry.o
#IFUNCS4 += sofs_ifuncs_4/soRenameDirEntry.o
#IFUNCS4 += sofs_ifuncs_4/soCheckDirectoryEmptiness.o

.PHONY: ifuncs1

all:			$(TARGET_LIB)

$(TARGET_LIB):		$(OBJS)
			ar -r $@ $^
			cp $@ ../../lib
			rm -f $^ $@

clean:
			rm -f $(OBJS) $(TARGET_LIB)
			rm -f ../../lib/$(TARGET_LIB)

