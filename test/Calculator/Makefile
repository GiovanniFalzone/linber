CC = g++
CCFLAGS = -std=c++11 -Wall -Wextra -pedantic -I.
LDLIBS = -lprotobuf -pthread

TARGET = Calculator_server Calculator_client
SOURCE = Calculator.pb.cc

.PHONY:
all: $(TARGET)

%: %.cpp
	protoc Calculator.proto --cpp_out=.
#	g++ -std=c++11 -o $@ $< Calculator.pb.cc $(CCFLAGS) $(LDLIBS)
	$(CC) -o $@ $< $(SOURCE) $(CCFLAGS) $(LDLIBS)

.PHONY:
clean:
	rm -f *.o $(TARGET)
	rm -f *.pb.*