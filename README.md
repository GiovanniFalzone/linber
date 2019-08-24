# Linber description
Linber is a RPC mechanism implemented as driver module in the linux kernel. It allow to register a service with its workers and then start requesting the service on demand just passing its service uri. \
Linber API uses IOCTL to comunicate with the Linber module through the file /dev/linber 		\

Each worker is waiting on the server semaphore, when a request arrive is insert in the FIFO queue and the workers are signaled for this event, take a slot and a request and start serving the request.


## Info
Distro: Ubuntu 7.4.0-1ubuntu1~18.04.1	\
Kernel version: 5.0.0-23-generic		\
gcc version 7.4.0						\

### Warning
The module uses 0x20 as magic number and [0-6] as sequence number to register the IOCTL operations. (https://elixir.bootlin.com/linux/latest/source/Documentation/ioctl/ioctl-number.txt)

### dependencies
	sudo apt-get install build-essential make libprotobuf-dev protobuf-compiler

## Execution
	./main.sh makeall	// compile all inside driver/ and test/
	./main.sh insmod	// load the module in the kernel
	./test/test_service/server org.service 4 10		// 4 workers, service execution time 10 milliseconds
	./test/test_service/client org.service 32 10	// 32 concurrent request (16 blocking, 16 non blocking), inter-request delay = random in [0, 10]
	./test/linber_monitor/linber_monitor 100 	// shot the system' status every 100 milliseconds
	./main.sh rmmod		// unload the module


## module test
	./test/test_service/server org.service 1 0	// service with 1 worker and 0 execution time
	./test/test_service/client_efficiency_test org.service 1 15		// run tests starting with a request of 1 byte and double for 15 times

In the x-axis is reported the size of each request used to test the framework, in the y-axis the microseconds calculated as the minimum one over 1000 requests with the same request.
The service server used just one worker and an execution time equal to 0, the response is a copy of the request.
![Linber Sequence Diagram](/img/test_exec_time.png)

## Linber API
![Linber Component view](/img/Linber_component_view.png)
![Linber Stack](/img/Linber_stack.png)

### Create Service

### Register Worker

### Worker Start Job

### Worker End Job

### Destroy Service

### Request Service


