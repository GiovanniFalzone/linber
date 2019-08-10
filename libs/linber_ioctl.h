#ifndef LINBER_IOCTL_H
#define LINBER_IOCTL_H

#include <linux/ioctl.h>
#define MAJOR_NUM	239

#define IOCTL_REGISTER_SERVICE	_IOR(MAJOR_NUM, 0, char*)
#define IOCTL_REQUEST_SERVICE	_IOW(MAJOR_NUM, 1, char*)

#define DEVICE_FILE_NAME		"linber"

#endif