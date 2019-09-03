CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LDLIBS = -pthread
TARGET = server_shm client_blocking_shm client_no_blocking_shm client_efficiency_test_shm

SRCS = ../../libs/linber_service_api.c

DEPS = ../../libs/linber_service_api.h
DEPS += ../../libs/linber_ioctl.h

.PHONY:
all: $(TARGET)

%: %.c $(DEPS) $(SRCS)
	sudo $(CC) -o $@ $< $(SRCS) $(CFLAGS) $(LDLIBS) 
	sudo chmod u+s $@

.PHONY:
clean:
	rm -f *.o $(TARGET)
