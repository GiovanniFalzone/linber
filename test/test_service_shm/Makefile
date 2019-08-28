CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LDLIBS = -pthread
TARGET = server_shm client_shm client_shm_efficiency_test

SRCS = ../../libs/linber_service_api.c

DEPS = ../../libs/linber_service_api.h
DEPS += ../../libs/linber_ioctl.h

.PHONY:
all: $(TARGET)

%: %.c $(DEPS) $(SRCS)
	$(CC) -o $@ $< $(SRCS) $(CFLAGS) $(LDLIBS) 


.PHONY:
clean:
	rm -f *.o $(TARGET)
