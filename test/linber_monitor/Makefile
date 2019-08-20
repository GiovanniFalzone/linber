CC = gcc
CFLAGS = -Wall -Wextra -pedantic -I.
TARGET = linber_monitor

.PHONY:
all: $(TARGET)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(LDLIBS)

.PHONY:
clean:
	rm -f *.o $(TARGET)