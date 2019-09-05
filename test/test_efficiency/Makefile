CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LDLIBS = -pthread

CLIENT_EFF_MEM = client_eff_mem
CLIENT_EFF_SHM = client_eff_shm

TARGET = $(CLIENT_EFF_MEM) $(CLIENT_EFF_SHM)

SRCS = ../../libs/linber_service_api.c

DEPS = ../../libs/linber_service_api.h
DEPS += ../../libs/linber_ioctl.h

.PHONY:
all: $(TARGET)

%: %.c $(DEPS) $(SRCS)
	sudo $(CC) -o $@ $< $(SRCS) $(CFLAGS) $(LDLIBS)
	sudo chmod u+s $@

.PHONY:
execute_test_mem:
	sudo nice -n -20 ./$(CLIENT_EFF_MEM)
	sudo chmod 666 *.csv

.PHONY:
execute_test_shm:
	sudo nice -n -20 ./$(CLIENT_EFF_SHM)
	sudo chmod 666 *.csv

.PHONY:
clean:
	rm -f *.o $(TARGET)
