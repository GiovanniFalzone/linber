# Linber description
Linber is a RPC mechanism implemented as driver module in the linux kernel.
Linber API uses IOCTL to comunicate with the Linber module through the file /dev/linber

for more deatails on the implementations take a look at linber.pdf.

## Info
### Used environment
Distro: Ubuntu 7.4.0-1ubuntu1~18.04.1	\
Kernel version: 5.0.0-23-generic		\
gcc version 7.4.0

The module uses 0xFF as identifier and [0-10] as sequence number to register the IOCTL operations. (https://elixir.bootlin.com/linux/latest/source/Documentation/ioctl/ioctl-number.txt)

### Dependencies
	sudo apt-get install build-essential make libprotobuf-dev protobuf-compiler

### Compilation
	make all					// build driver module and tests
	make tests					// compile all tests
	make insmod					// load the module in the kernel
	make rmmod					// remove the module
	make updatemod					// remove, build and reload the module

## Root permissions
In order to set SCHED_DEADLINE as scheduler when required the server must run as root.
Also the the client need to execut as root when the server is using the shared memory otherwise the it will not be able to execute shmctl on the share memory created by the server and the shared memory it is not removed from the system.
This is a temporary condition, the test's Makefile will compile the client and the server as root and then set the SUID bit for the binary files.

## module test using kernel memory to transfer the payload
	//------- service with 1 worker, 5 ms execution time, 100 ms of period and 2 request per period (P=100, Q=2*exec_time) used for SCHED_DEADLINE
	./test/test_service/server org.service 1 5 100 2

	//------- client with 4 concurrent request (threads), delayed on creation by a random value in range [1, 10]ms, with a relative deadline of 1000ms
	// the random delay on creation allow to simulate the general case between periodic and sporadic requests
	./test/test_service/client_blocking org.service 4 10 1000

	//------- same as the latter but this is a non blocking request, the new last parameter is the waiting time before asking for the response
	./test/test_service/client_no_blocking org.service 4 10 1000 100

If the relative deadline is equal to 0 the request will be dispatched only if the incoming queue is empty or there aren't Realtime requets
When a request without Real Time constraints is dispatched a Best Effor policy is applied.
When a Real Time request is dispatched a SCHED_DEADLINE policy is applyied to the serving worker using the Service Period and Budget but setting as deadline the minimum between the service period and the relative deadline at dispatch moment, if the deadline is expired then the Best Effort policy is applied.
