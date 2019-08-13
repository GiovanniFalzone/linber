CC = gcc
CFLAGS = -Wall -pedantic -I.
LDLIBS = -pthread

TARGET = client server

.PHONY:
all: $(TARGET)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(LDLIBS)

.PHONY:
clean:
	rm -f *.o $(TARGET)
