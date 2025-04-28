#ifndef SERVO_DEVICE_H
#define SERVO_DEVICE_H

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/pwm.h>
#include <linux/types.h>

typedef struct  {
  struct device *device;
  struct pwm_device *pwm[2];
  dev_t device_number;
  struct cdev cdev;
} ServoDevice;

#endif  // SERVO_DEVICE_H
