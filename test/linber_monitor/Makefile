CC = gcc
CFLAGS = -Wall -Wextra -pedantic

INCLUDE_DIR = -I../../libs/inc
CFLAGS += $(INCLUDE_DIR)

LDLIBS = -pthread

TARGET = linber_monitor

SRCS = ../../libs/src/linber_service_api.c

DEPS = ../../libs/inc/linber_service_api.h
DEPS += ../../libs/inc/linber_ioctl.h

.PHONY:
all: $(TARGET)

%: %.c $(DEPS) $(SRCS)
	$(CC) -o $@ $< $(SRCS) $(CFLAGS) $(LDLIBS) 

.PHONY:
clean:
	rm -f *.o $(TARGET)
