#ifndef LOG_H
#define LOG_H

#include <linux/kernel.h>

#define SERVO_DRIVER_DEBUG 1

#undef LOG_ERROR
#ifdef __KERNEL__
// Debugging is on in kernel space
#define LOG_ERROR(fmt, args...) printk(KERN_ERR "servo_driver_module: " fmt "\n", ##args)
#else
// Debugging is on in user space
#define LOG_ERROR(fmt, args...) fprintf(stderr, "ERROR: " fmt "\n", ##args)
#endif

#undef LOG_WARN
#ifdef __KERNEL__
// Debugging is on in kernel space
#define LOG_WARN(fmt, args...) printk(KERN_WARNING "servo_driver_module: " fmt "\n", ##args)
#else
// Debugging is on in user space
#define LOG_WARN(fmt, args...) fprintf(stderr, "WARNING: " fmt "\n", ##args)
#endif

#undef LOG_DEBUG /* undef it, just in case */
#ifdef SERVO_DRIVER_DEBUG
// Debugging is on

#ifdef __KERNEL__
// Debugging is on in kernel space
#define LOG_DEBUG(fmt, args...) printk(KERN_DEBUG "servo_driver_module: " fmt "\n", ##args)
#else
// Debugging is on in user space
#define LOG_DEBUG(fmt, args...) fprintf(stderr, "DEBUG: " fmt "\n", ##args)
#endif

#else
// Debugging is off
#define LOG_DEBUG(fmt, args...) // define as nothing
#endif

#endif  // LOG_H
