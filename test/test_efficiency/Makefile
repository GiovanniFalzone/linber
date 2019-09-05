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
execute_test: execute_test_mem execute_test_shm

.PHONY:
execute_test_mem:
#	./../test_service/server org.service 1 0 && echo $$	
	sudo nice -n -20 ./$(CLIENT_EFF_MEM)
	sudo killall server org.service 1 0

.PHONY:
execute_test_shm:
#	./../test_service_shm/server_shm org.service 1 0
	sudo nice -n -20 ./$(CLIENT_EFF_SHM)
	sudo killall server_shm org.service 1 0

.PHONY:
help:
	@echo "[all] | [clean] | [execute_test]"

.PHONY:
clean:
	rm -f *.o $(TARGET)
