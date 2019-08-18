# Linber
Linber is a RPC mechanism implemented as driver module in the linux kernel. It allow to register a service with its workers and then start requesting the service on demand just passing its service uri. \
Linber API uses IOCTL to comunicate with the Linber module through the file /dev/linber 		\

## Create Service
- ***Input:***
	- *service uri* to identify the service	
	- *uri len*		string length of the uri
	- *exec time*	time needed to compute the service
	- *max workers*	maximum number of workers working on parallel requests
- ***Output:***
	- *service token* 64 bit assigned by the module to check the other requests

## Register Worker
linber_register_service_worker(char * service_uri, unsigned int uri_len, unsigned int * worker_id)
- ***Input:***
	- *service uri*
	- *uri len*
- ***Output:***
	- *worker id*

## Worker Start Job
- **Input:**
- **Output:**

## Worker End Job
- **Input:**
- **Output:**

## Destroy Service
- **Input:**
- **Output:**

## Request Service
- **Input:**
- **Output:**

### Info and howto
Distro: Ubuntu 7.4.0-1ubuntu1~18.04.1\
Kernel version: 5.0.0-23-generic\
gcc version 7.4.0\

./main.sh makeall	// clean and compile inside driver/ and test/ \
./main.sh insmod	// load the module in the kernel \


![Linber Component view](/img/Linber_component_view.png)
![Linber Stack](/img/Linber_stack.png)

