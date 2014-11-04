CC = gcc
CFLAGS  = -Wall 
CFLAGS += -D_FILE_OFFSET_BITS=64
CFLAGS += -I "../debugging" -I "../rawIO14" -I "../sofs14"

LIB_NAME = syscalls14

TARGET_LIB = lib$(LIB_NAME).a

#OBJS = soRead.o
#OBJS += soReaddir.o
#OBJS += soRename.o
#OBJS += soTruncate.o
#OBJS += soLink.o

all:			$(TARGET_LIB)

$(TARGET_LIB):		$(OBJS)
			ar -r $@ $^
			cp $@ ../../lib
			rm -f $^ $@

clean:
			rm -f $(OBJS) $(TARGET_LIB)
			rm -f ../../lib/$(TARGET_LIB)

