CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LDLIBS = -pthread

TARGET = server client_blocking client_no_blocking client_efficiency_test

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
