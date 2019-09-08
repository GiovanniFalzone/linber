CC = gcc
CFLAGS = -Wall -Wextra -pedantic

INCLUDE_DIR = -I../../libs/inc
CFLAGS += $(INCLUDE_DIR)

LDLIBS = -pthread

CLIENT_EFF_MEM = client_eff_mem
CLIENT_EFF_SHM = client_eff_shm

TARGET = $(CLIENT_EFF_MEM) $(CLIENT_EFF_SHM)

SRCS = ../../libs/src/linber_service_api.c

DEPS = ../../libs/inc/linber_service_api.h
DEPS += ../../libs/inc/linber_ioctl.h

.PHONY:
all: $(TARGET)

%: %.c $(DEPS) $(SRCS)
	$(CC) -o $@ $< $(SRCS) $(CFLAGS) $(LDLIBS)

.PHONY:
execute_test_mem:
	sudo nice -n -20 ./$(CLIENT_EFF_MEM)

.PHONY:
execute_test_shm:
	sudo nice -n -20 ./$(CLIENT_EFF_SHM)

.PHONY:
clean:
	rm -f *.o $(TARGET)
