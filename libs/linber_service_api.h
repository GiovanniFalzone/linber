#ifndef LINBER_SERVICE_API_H
#define LINBER_SERVICE_API_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include "linber_ioctl.h"

#define LINBER_ERROR_DEVICE_FILE	-1
#define LINBER_ERROR_URI			-2

#define DEBUG_LINBER

int linber_device_file_desc = -1;

int ioctl_register_service(int file_desc, linber_register_service_struct param){
	int ret;
	printf("registering service: %s\n", param.service_uri);
	ret = ioctl(file_desc, IOCTL_REGISTER_SERVICE, param);
	if(ret < 0){
		printf("ioctl_register_service failerd:%d\n", ret);
		return -1;
	}
}

int ioctl_request_service(int file_desc, linber_request_service_struct param){
	int ret;
	printf("requesting service: %s\n", param.service_uri);
	ret = ioctl(file_desc, IOCTL_REQUEST_SERVICE, param);
	if(ret < 0){
		printf("ioctl_request_service failerd:%d\n", ret);
		return -1;
	}
}

//------------------------------------------
int linber_init(){
	linber_device_file_desc = open(DEVICE_FILE_FULL_PATH, 0);
	if(linber_device_file_desc < 0){
		printf("Can't open %s\n", DEVICE_FILE_FULL_PATH);
		return LINBER_ERROR_DEVICE_FILE;
	}
}

int linber_exit(){
	if(linber_device_file_desc < 0){
		printf("Linber Exit: device file error\n");
		return LINBER_ERROR_DEVICE_FILE;
	}
	close(linber_device_file_desc);
}

int linber_register_service(char * service_uri, uint uri_len, uint exectime){
	if(linber_device_file_desc < 0){
		printf("Linber Register: device file error\n");
		return LINBER_ERROR_DEVICE_FILE;
	}
	if(uri_len > SERVICE_URI_MAX_LEN){
		printf("Linber Register: uri size too large\n");
		return LINBER_ERROR_URI;
	}
	linber_register_service_struct param;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.service_exectime = exectime;
	ioctl_register_service(linber_device_file_desc, param);
}

int linber_request_service(char * service_uri, uint uri_len, uint rel_deadline){
	if(linber_device_file_desc < 0){
		printf("Linber Request: device file error\n");
		return LINBER_ERROR_DEVICE_FILE;
	}
	if(uri_len > SERVICE_URI_MAX_LEN){
		printf("Linber Request: uri size too large\n");
		return LINBER_ERROR_URI;
	}
	linber_request_service_struct param;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.request_rel_deadline = rel_deadline;
	ioctl_request_service(linber_device_file_desc, param);
}


#endif