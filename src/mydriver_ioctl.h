#ifndef __DRIVER_H
#define __DRIVER_H

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

#define DRIVER_IOC_MAGIC  'k'
#define DRIVER_IOCRESET    _IO(DRIVER_IOC_MAGIC, 0)
#define DRIVER_IOC_MAXNR 12

#endif