# Linber
Linber is a RPC mechanism implemented as driver module in the linux kernel. It allow to register a service with its workers and then start requesting the service on demand just passing its service uri. \
Linber API uses IOCTL to comunicate with the Linber module through the file /dev/linber 		\

## Create Service
- ***Input:***
	- *service uri*, to identify the service	
	- *uri length*,		string length of the uri
	- *exec time*,	time needed to compute the service
	- *max workers*,	maximum number of workers working on parallel requests
- ***Output:***
	- *service token*, 64 bit assigned by the module to check the other requests

## Register Worker
- ***Input:***
	- *service uri*, to identify the service	
	- *uri len*, string length of the uri
	- *service token*, for safety reason
- ***Output:***
	- *worker id*, assigned worker id, minimum value 0

## Worker Start Job
- **Input:**
	- *service uri*, to identify the service	
	- *uri len*, string length of the uri
	- *service token*, for safety reason
	- *worker id*, assigned worker id, minimum value 0
- **Output:**
	- *slot id*, assigned slot id where the request is stored
	- *service parameters*, string containing the application level service's parameters
	- *service parameters length*, lenght of the service's parameters string

## Worker End Job
- **Input:**
	- *service uri*, to identify the service	
	- *uri len*, string length of the uri
	- *service token*, for safety reason
	- *worker id*, assigned worker id, minimum value 0
	- *slot id*, assigned slot id where the request is stored
	- *service result*, string containing the application level service's parameters
	- *service result length*, lenght of the service's parameters string

## Destroy Service
- **Input:**
	- *service uri*, to identify the service	
	- *uri len*, string length of the uri
	- *service token*, for safety reason

## Request Service
- **Input:**
	- *service uri*, to identify the service	
	- *uri len*, string length of the uri
	- *service parameters*, string containing the application level service's parameters
	- *service parameters length*, lenght of the service's parameters string
- **Output:**
	- *service result*, string containing the application level service's parameters
	- *service result length*, lenght of the service's parameters string


### Info and howto
Distro: Ubuntu 7.4.0-1ubuntu1~18.04.1\
Kernel version: 5.0.0-23-generic\
gcc version 7.4.0\

./main.sh makeall	// clean and compile inside driver/ and test/ \
./main.sh insmod	// load the module in the kernel \


![Linber Component view](/img/Linber_component_view.png)
![Linber Stack](/img/Linber_stack.png)

