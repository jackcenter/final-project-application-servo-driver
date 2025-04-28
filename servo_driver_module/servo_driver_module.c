#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/uaccess.h>

#include "log.h"
#include "servo_device.h"
#include "servo_ioctl.h"

MODULE_AUTHOR("Jack Center");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Creates a character driver for controlling a servo");

#define DEVICE_NAME "servo_driver"
#define DEVICE_CNT 2

static u32 pwm_period_ns = 20000000;
static u32 pwm_duty_cycle_ns = 1500000;

static int pwm_probe(struct platform_device *pdev);
static int pwm_remove(struct platform_device *pdev);
static long servo_driver_ioctl(struct file *file_p, unsigned int cmd,
                               unsigned long arg);
static int servo_driver_open(struct inode *inode, struct file *file_p);
static int servo_driver_release(struct inode *inode, struct file *file_p);
static ssize_t servo_driver_read(struct file *file_p, char __user *buffer,
                                 size_t len, loff_t *offset);
static ssize_t servo_driver_write(struct file *file_p,
                                  const char __user *buffer, size_t len,
                                  loff_t *offset);

static int clamp_value(const int val, const int min, const int max);

static int map_value(const int val, const int in_min, const int in_max,
                     const int out_min, const int out_max);

static const struct of_device_id pwm_dt_ids[] = {
    {.compatible = "mycompany,my-pwm-device"}, {}}; // From device tree property
MODULE_DEVICE_TABLE(of, pwm_dt_ids);                // Matches driver to device

static struct platform_driver pwm_driver = {
    .probe = pwm_probe,
    .remove = pwm_remove,
    .driver =
        {
            .name = DEVICE_NAME,
            .of_match_table = pwm_dt_ids,
        },
};

static const struct file_operations servo_driver_fops = {
    .owner = THIS_MODULE,
    .open = servo_driver_open,
    .read = servo_driver_read,
    .release = servo_driver_release,
    .write = servo_driver_write,
    .unlocked_ioctl = servo_driver_ioctl,
};

module_platform_driver(pwm_driver); // Handles init and exit

int pwm_probe(struct platform_device *pdev) {
  LOG_DEBUG("pwm_probe");

  ServoDevice *servo_device;
  servo_device = devm_kzalloc(&pdev->dev, sizeof(*servo_device), GFP_KERNEL);
  if (!servo_device) {
    return -ENOMEM;
  }

  servo_device->device = &pdev->dev;

  int result =
      alloc_chrdev_region(&servo_device->device_number, 0, DEVICE_CNT, DEVICE_NAME);
  if (result < 0) {
    LOG_ERROR("alloc_chrdev_region");
    return result;
  }

  cdev_init(&servo_device->cdev, &servo_driver_fops);
  servo_device->cdev.owner = THIS_MODULE;

  result = cdev_add(&servo_device->cdev, servo_device->device_number, DEVICE_CNT);
  if (result < 0) {
    LOG_ERROR("cdev_add");
    unregister_chrdev_region(servo_device->device_number, DEVICE_CNT);
    return result;
  }

  servo_device->pwm[0] = devm_pwm_get(servo_device->device, "pwm0");
  if (IS_ERR(servo_device->pwm[0])) {
    int err = PTR_ERR(servo_device->pwm[0]);
    if (err == -EPROBE_DEFER) {
      LOG_WARN("PWM not ready, deferring probe 0");
      unregister_chrdev_region(servo_device->device_number, DEVICE_CNT);
      return -EPROBE_DEFER;
    }
    LOG_ERROR("Failed to get PWM: %d", err);
    unregister_chrdev_region(servo_device->device_number, DEVICE_CNT);
    return err;
  }
  pwm_config(servo_device->pwm[0], pwm_duty_cycle_ns, pwm_period_ns);

  servo_device->pwm[1] = devm_pwm_get(servo_device->device, "pwm1");
  if (IS_ERR(servo_device->pwm[1])) {
    int err = PTR_ERR(servo_device->pwm[1]);
    if (err == -EPROBE_DEFER) {
      LOG_WARN("PWM not ready, deferring probe 1");
      unregister_chrdev_region(servo_device->device_number, DEVICE_CNT);
      return -EPROBE_DEFER;
    }
    LOG_ERROR("Failed to get PWM: %d", err);
    unregister_chrdev_region(servo_device->device_number, DEVICE_CNT);
    return err;
  }
  pwm_config(servo_device->pwm[1], pwm_duty_cycle_ns, pwm_period_ns);

  platform_set_drvdata(pdev, servo_device);

  return 0;
}

int pwm_remove(struct platform_device *pdev) {
  LOG_DEBUG("pwm_remove");
  ServoDevice *servo_device = platform_get_drvdata(pdev);
  if (!servo_device) {
    pr_err("servo_driver: pwm_remove called with NULL drvdata\n");
    return -EINVAL;
}

  if (servo_device->pwm[0]) {
    pwm_disable(servo_device->pwm[0]);
  }
  
  if (servo_device->pwm[1]) {
    pwm_disable(servo_device->pwm[1]);
  }
  
  cdev_del(&servo_device->cdev);
  unregister_chrdev_region(servo_device->device_number, DEVICE_CNT);

  return 0;
}

static int servo_driver_open(struct inode *inode, struct file *file_p) {
  LOG_DEBUG("servo_driver_open");

  ServoDevice *servo_device = container_of(inode->i_cdev, ServoDevice, cdev);
  unsigned int minor = MINOR(inode->i_rdev);

  file_p->private_data = servo_device;
  file_p->f_inode->i_private = (void *)(uintptr_t)minor;
  
  return 0;
}

static int servo_driver_release(struct inode *inode, struct file *file_p) {
  LOG_DEBUG("servo_driver_release");
  return 0;
}

static long servo_driver_ioctl(struct file *file_p, unsigned int cmd,
                               unsigned long arg) {
  LOG_DEBUG("servo_driver_ioctl");

  ServoDevice *servo_device = file_p->private_data;
  unsigned int minor = iminor(file_p->f_inode);

  switch (cmd) {
  case SERVO_ENABLE:
    pwm_enable(servo_device->pwm[minor]);
    return 0;
  case SERVO_DISABLE:
    pwm_disable(servo_device->pwm[minor]);
    return 0;
  default:
    LOG_WARN("unhandled ioctl command: %u", cmd);
    return -EINVAL;
  }
}

static ssize_t servo_driver_read(struct file *file_p, char __user *buffer,
                                 size_t len, loff_t *offset) {
  if (*offset > 0)
    return 0;

  ServoDevice *servo_device = file_p->private_data;
  unsigned int minor = iminor(file_p->f_inode);

  struct pwm_state state;
  pwm_get_state(servo_device->pwm[minor], &state);
  
  int position = 0;
  // check if enabled, if not, write -1
  if (!state.enabled) {
    position = -1;
  } else {
    position = map_value(state.duty_cycle, 500000, 2300000, 0, 180);
  }

  char buf[16];  // Space for 32 bit int
  const int len_written = snprintf(buf, sizeof(buf), "%d\n", position);

  if (len_written < 0)
    return -EFAULT;

  // Only copy as much as user space can take
  if (len_written > len)
    len_written = len;
    
  if (copy_to_user(buffer, buf, len_written))
    return -EFAULT;

  *offset += len_written;
  return len_written;
}

static ssize_t servo_driver_write(struct file *file_p,
                                  const char __user *buffer, size_t len,
                                  loff_t *offset) {
  ServoDevice *servo_device = file_p->private_data;
  unsigned int minor = iminor(file_p->f_inode);

  char buf[16];  // Space for 32 bit int
  if (len >= sizeof(buf)) {
    return -EINVAL;
  }

  if (copy_from_user(buf, buffer, len)) {
    return -EFAULT;
  }
  buf[len] = '\0'; // make the input a string

  int val;
  int retval = kstrtoint(buf, 10 /* base */, &val);
  if (retval) {
    LOG_ERROR("kstrtoint (%d)", retval);
    return -EINVAL;
  }

  if (val < 0 || val > 180) {
    LOG_ERROR("servo command (%d) is out of range [%d, %d]", val, 0, 180);
    return -EINVAL;
  }

  int pwm_duty_cycle_ns = map_value(val, 0, 180, 500000, 2300000);
  pwm_config(servo_device->pwm[minor], pwm_duty_cycle_ns, pwm_period_ns);
  return len;
}

int clamp_value(const int val, const int min, const int max) {
  if (val < min) {
    return min;
  }

  if (val > max) {
    return max;
  }

  return val;
}

int map_value(const int val, const int in_min, const int in_max,
              const int out_min, const int out_max) {
  if (in_min == in_max) {
    LOG_WARN("Input range [%d, %d] is zero width. Returning minimum value from "
             "output range.",
             in_min, in_max);
    return out_min;
  }

  int clamped_val = clamp_value(val, in_min, in_max);
  if (clamped_val != val) {
    LOG_WARN("Mapped value (%d) was clamped to range [%d, %d].", val, in_min,
             in_max);
  }

  return (clamped_val - in_min) * (out_max - out_min) / (in_max - in_min) +
         out_min;
}
