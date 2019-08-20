CC = gcc
CFLAGS = -Wall -Wextra -pedantic -I.
TARGET = linber_monitor
DEP_LIBS = ../../libs/linber_ioctl.h
DEP_LIBS += ../../libs/linber_service_api.h

.PHONY:
all: $(TARGET)

%.o: %.c $(DEP_LIBS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LDLIBS)

.PHONY:
clean:
	rm -f *.o $(TARGET)