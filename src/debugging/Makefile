CC = gcc
CFLAGS = -Wall

LIB_NAME = debugging

TARGET_LIB = lib$(LIB_NAME).a

OBJS = sofs_probe.o

all:			$(TARGET_LIB)

$(TARGET_LIB):		$(OBJS)
			ar -r $(TARGET_LIB) $^
			cp $(TARGET_LIB) ../../lib
			rm -f $^ $(TARGET_LIB)

clean:
			rm -f $(OBJS) $(TARGET_LIB)
			rm -f ../../lib/$(TARGET_LIB)

