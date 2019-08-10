#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ioctl.h>

#include "../libs/linber_ioctl.h"

int ioctl_register_service(int file_desc, char * param){
	int ret;
	ret = ioctl(file_desc, IOCTL_REGISTER_SERVICE, param);
	if(ret < 0){
		printf("ioctl_register_service failerd:%d\n", ret);
		return -1;
	}
}

int ioctl_request_service(int file_desc, char * param){
	int ret;
	ret = ioctl(file_desc, IOCTL_REQUEST_SERVICE, param);
	if(ret < 0){
		printf("ioctl_request_service failerd:%d\n", ret);
		return -1;
	}
}

int main(){
	int file_desc, ret;
	char *service_name = "babba";
	printf("Running client test\n");
	file_desc = open("/dev/linber", 0);
	if(file_desc < 0){
		printf("Can't open %s\n", "/dev/linber");
		exit(-1);
	}
	ioctl_register_service(file_desc, service_name);
	ioctl_request_service(file_desc, service_name);
	close(file_desc);
}