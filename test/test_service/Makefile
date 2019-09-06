CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LDLIBS = -pthread

INCLUDE_DIR = -I../../libs/inc
CFLAGS += $(INCLUDE_DIR)

TARGET = server client_blocking client_no_blocking

SRCS = ../../libs/src/linber_service_api.c

DEPS = ../../libs/inc/linber_service_api.h
DEPS += ../../libs/inc/linber_ioctl.h

.PHONY:
all: $(TARGET)

%: %.c $(DEPS) $(SRCS)
	sudo $(CC) -o $@ $< $(SRCS) $(CFLAGS) $(LDLIBS)
	sudo chmod u+s $@


.PHONY:
clean:
	rm -f *.o $(TARGET)
