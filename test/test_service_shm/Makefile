CC = gcc
CFLAGS = -Wall -Wextra -pedantic -I.
LDLIBS = -pthread
TARGET = server_shm client_shm client_shm_efficiency_test
DEP_LIBS = ../../libs/linber_ioctl.h
DEP_LIBS += ../../libs/linber_service_api.h

.PHONY:
all: $(TARGET)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(LDLIBS)

.PHONY:
clean:
	rm -f *.o $(TARGET)