CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LDLIBS = -pthread

INCLUDE_DIR = -I../../libs/inc
CFLAGS += $(INCLUDE_DIR)

TARGET = server_shm client_blocking_shm client_no_blocking_shm

SRCS = ../../libs/src/linber_service_api.c

DEPS = ../../libs/inc/linber_service_api.h
DEPS += ../../libs/inc/linber_ioctl.h

.PHONY:
all: $(TARGET)

%: %.c $(DEPS) $(SRCS)
	$(CC) -o $@ $< $(SRCS) $(CFLAGS) $(LDLIBS)

server_shm: server_shm.c $(DEPS) $(SRCS)
	sudo $(CC) -o $@ $< $(SRCS) $(CFLAGS) $(LDLIBS)
	sudo chmod u+s $@

.PHONY:
clean:
	rm -f *.o $(TARGET)
