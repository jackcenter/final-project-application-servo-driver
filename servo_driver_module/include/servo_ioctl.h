#ifndef SERVO_IOCTL_H
#define SERVO_IOCTL_H

#ifdef __KERNEL__
#include <asm-generic/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
#endif

// Pick an arbitrary unused value from https://github.com/torvalds/linux/blob/master/Documentation/userspace-api/ioctl/ioctl-number.rst
#define SERVO_IOCTL_MAGIC 0x16

#define SERVO_ENABLE _IO(SERVO_IOCTL_MAGIC, 0)
#define SERVO_DISABLE _IO(SERVO_IOCTL_MAGIC, 1)

#endif  // SERVO_IOCTL_H
