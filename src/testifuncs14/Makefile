CC = gcc
CFLAGS = -Wall -I "../debugging" -I "../rawIO14" -I "../sofs14" -I.

TARGET = testifuncs14

SUFFIX = $(shell getconf LONG_BIT)

LIBS =
LIBS += -lsofs14
LIBS += -lsofs14bin_$(SUFFIX)
LIBS += -lrawIO14bin_$(SUFFIX)
LIBS += -lrawIO14
LIBS += -ldebugging

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

