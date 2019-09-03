CC = g++
CCFLAGS = -std=c++11 -Wall -Wextra -pedantic -I.
LDLIBS = -lprotobuf -pthread

TARGET = Calculator.pb Calculator_server Calculator_client

SRCS = ../../libs/linber_service_api.c
SRCS += Calculator.pb.cc

DEPS = ../../libs/linber_service_api.h
DEPS += ../../libs/linber_ioctl.h
DEPS += ../../libs/linberServiceWorker.h
DEPS += ../../libs/linberClient.h

PROTO = Calculator.proto

.PHONY:
all: $(TARGET)

%: %.cpp $(DEPS) $(PROTO) 
	sudo $(CC) -o $@ $< $(SRCS) $(CCFLAGS) $(LDLIBS) 
	sudo chmod u+s $@

Calculator.pb:
	protoc $(PROTO) --cpp_out=.


.PHONY:
clean:
	rm -f *.o $(TARGET)
	rm -f *.pb.*
