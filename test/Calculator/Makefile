CC = g++
CCFLAGS = -std=c++11 -Wall -Wextra -pedantic -I.

INCLUDE_DIR = -I../../libs/inc
CCFLAGS += $(INCLUDE_DIR)

LDLIBS = -lprotobuf -pthread

TARGET = Calculator.pb server client

SRCS = 	../../libs/src/linber_service_api.c
SRCS += ../../libs/src/linberServiceWorker.cpp
SRCS += ../../libs/src/linberServiceClient.cpp
SRCS += Calculator.pb.cc
SRCS += Calculator_service_worker.cpp
SRCS += Calculator_service_client.cpp

DEPS = 	../../libs/inc/linber_service_api.h
DEPS += ../../libs/inc/linber_ioctl.h
DEPS += ../../libs/inc/linberServiceClient.h
DEPS += ../../libs/inc/linberServiceWorker.h
DEPS += Calculator_service_client.h



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
