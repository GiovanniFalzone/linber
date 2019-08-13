#ifndef LINBER_IOCTL_H
#define LINBER_IOCTL_H

#include <linux/ioctl.h>
#define MAJOR_NUM	239

#define IOCTL_REGISTER_SERVICE	_IOR(MAJOR_NUM, 0, char*)
#define IOCTL_REQUEST_SERVICE	_IOW(MAJOR_NUM, 1, char*)
#define IOCTL_START_JOB_SERVICE	_IOW(MAJOR_NUM, 2, char*)
#define IOCTL_END_JOB_SERVICE	_IOW(MAJOR_NUM, 3, char*)



#define DEVICE_FILE_NAME		"linber"
#define DEVICE_FILE_FULL_PATH	"/dev/" DEVICE_FILE_NAME

#define SERVICE_URI_MAX_LEN		50

typedef struct linber_service_struct {
	char *service_uri;
	unsigned int service_uri_len;
	unsigned int service_time;
} linber_service_struct;

#endif