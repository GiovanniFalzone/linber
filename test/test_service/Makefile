CC = gcc
CFLAGS = -Wall -Wextra -pedantic -I.
LDLIBS = -pthread
TARGET = server client

.PHONY:
all: $(TARGET)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(LDLIBS)

.PHONY:
clean:
	rm -f *.o $(TARGET)