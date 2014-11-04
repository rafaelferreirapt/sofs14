CC = gcc
CFLAGS  = -Wall 
CFLAGS += -D_FILE_OFFSET_BITS=64 
CFLAGS += -DFUSE_USE_VERSION=26 
CFLAGS += -I "../debugging" -I "../rawIO14" 
CFLAGS += -I "../sofs14" -I "../syscalls14"

TARGET = mount_sofs14

SUFFIX = $(shell getconf LONG_BIT)

LIBS =
LIBS += -lsyscalls14
LIBS += -lsyscalls14bin_$(SUFFIX)
LIBS += -lsofs14
LIBS += -lsofs14bin_$(SUFFIX)
LIBS += -lrawIO14bin_$(SUFFIX)
LIBS += -lrawIO14
LIBS += -ldebugging
LIBS += -lpthread
LIBS += -lfuse

LFLAGS = -L "../../lib" $(LIBS)

OBJS = 

all:		$(TARGET)

$(TARGET):	$(TARGET).o $(OBJS)
			$(CC) -o $@ $^ $(LFLAGS)
			cp $@ ../../run
			rm -f $^ $@

clean:
			rm -f $(TARGET) $(TARGET).o $(OBJS)
			rm -f ../../run/$(TARGET)
