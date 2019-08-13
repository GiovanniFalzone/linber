CC = gcc
CFLAGS = -Wall -pedantic -I.

TARGET = client server

.PHONY:
all: $(TARGET)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY:
clean:
	rm -f *.o $(TARGET)