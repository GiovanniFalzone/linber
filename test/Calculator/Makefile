CC = g++
CCFLAGS = -std=c++11 -Wall -Wextra -pedantic -I.
LDLIBS = -lprotobuf -pthread
DEP_LIBS = ../../libs/linber_ioctl.h
DEP_LIBS += ../../libs/linber_service_api.h
DEP_LIBS += ../../libs/linberServiceServer.h
DEP_LIBS += ../../libs/linberServiceWorker.h

TARGET = Calculator_server Calculator_client
SOURCE = Calculator.pb.cc

.PHONY:
all: $(TARGET)

%: %.cpp $(DEP_LIBS)
	protoc Calculator.proto --cpp_out=.
	$(CC) -o $@ $< $(SOURCE) $(CCFLAGS) $(LDLIBS)

.PHONY:
clean:
	rm -f *.o $(TARGET)
	rm -f *.pb.*