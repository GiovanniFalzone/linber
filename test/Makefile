CC = gcc
CFLAGS = -Wall -pedantic -I.
LDLIBS = -pthread

TARGET = linber_monitor/monitor
TARGET += test_service/client test_service/server

.PHONY:
all: $(TARGET)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(LDLIBS)

.PHONY:
clean:
	rm -f *.o $(TARGET)
