#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/uaccess.h>

#include "log.h"
#include "servo_ioctl.h"

MODULE_AUTHOR("Jack Center");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Creates a character driver for controlling a servo");

#define DEVICE_NAME "servo_driver"

static int major_number;
static struct cdev servo_cdev;

static struct pwm_device *pwm0;
static struct pwm_device *pwm1;

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
  dev_t dev;
  int result;

  LOG_DEBUG("pwm_probe");

  result = alloc_chrdev_region(&dev, 0, 2, DEVICE_NAME);
  if (result < 0) {
    LOG_ERROR("Failed to allocate char device region");
    return result;
  }

  major_number = MAJOR(dev);

  cdev_init(&servo_cdev, &servo_driver_fops);
  result = cdev_add(&servo_cdev, dev, 2);
  if (result < 0) {
    LOG_ERROR("Failed to add cdev");
    unregister_chrdev_region(dev, 2);
    return result;
  }

  pwm0 = devm_pwm_get(&pdev->dev, "pwm0");
  if (IS_ERR(pwm0)) {
    int err = PTR_ERR(pwm0);
    if (err == -EPROBE_DEFER) {
      LOG_WARN("PWM not ready, deferring probe 0");
      unregister_chrdev_region(dev, 2);
      return -EPROBE_DEFER;
    }
    LOG_ERROR("Failed to get PWM 0: %d", err);
    unregister_chrdev_region(dev, 2);
    return err;
  }

  pwm1 = devm_pwm_get(&pdev->dev, "pwm1");
  if (IS_ERR(pwm1)) {
    int err = PTR_ERR(pwm1);
    if (err == -EPROBE_DEFER) {
      LOG_WARN("PWM not ready, deferring probe 1");
      unregister_chrdev_region(dev, 2);
      return -EPROBE_DEFER;
    }
    LOG_ERROR("Failed to get PWM 1: %d", err);
    unregister_chrdev_region(dev, 2);
    return err;
  }

  pwm_config(pwm0, pwm_duty_cycle_ns, pwm_period_ns);
  pwm_config(pwm1, pwm_duty_cycle_ns, pwm_period_ns);

  return 0;
}

int pwm_remove(struct platform_device *pdev) {
  dev_t dev = MKDEV(major_number, 0);

  LOG_DEBUG("pwm_remove");
  pwm_disable(pwm0);
  pwm_disable(pwm1);
  cdev_del(&servo_cdev);
  unregister_chrdev_region(dev, 2);

  return 0;
}

static int servo_driver_open(struct inode *inode, struct file *file_p) {
  LOG_DEBUG("servo_driver_open");
  return 0;
}

static int servo_driver_release(struct inode *inode, struct file *file_p) {
  LOG_DEBUG("servo_driver_release");
  return 0;
}

static long servo_driver_ioctl(struct file *file_p, unsigned int cmd,
                               unsigned long arg) {
  LOG_DEBUG("servo_driver_ioctl");

  switch (cmd) {
  case SERVO_ENABLE:
    pwm_enable(pwm0);
    return 0;
  case SERVO_DISABLE:
    pwm_disable(pwm0);
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

  

  struct pwm_state state;
  pwm_get_state(pwm0, &state);

  int position = 0;
  // check if enabled, if not, write -1
  if (!state.enabled) {
    position = -1;
  } else {
    position = map_value(state.duty_cycle, 500000, 2300000, 0, 180);
  }

  // map value to position

  char buf[16];
  const int len_written = snprintf(buf, sizeof(buf), "%u\n", position);

  if (copy_to_user(buffer, buf, len_written))
    return -EFAULT;

  *offset += len_written;
  return len_written;
}

static ssize_t servo_driver_write(struct file *file_p,
                                  const char __user *buffer, size_t len,
                                  loff_t *offset) {
  char buf[16];

  if (len >= sizeof(buf))
    return -EINVAL;

  if (copy_from_user(buf, buffer, len))
    return -EFAULT;

  buf[len] = '\0'; // make the input a string

  int val;
  int retval = kstrtoint(buf, 10, &val);
  if (retval) {
    LOG_ERROR("kstrtoint (%d)", retval);
    return -EINVAL;
  }

  // check range: should go in a config
  if (val < 0 || val > 180) {
    LOG_ERROR("servo command (%d) is out of range [%d, %d]", val, 0, 180);
    return -EINVAL;
  }

  // covert to pwm
  int pwm_duty_cycle_ns = map_value(val, 0, 180, 500000, 2300000);

  // write to pwm
  pwm_config(pwm0, pwm_duty_cycle_ns, pwm_period_ns);
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
