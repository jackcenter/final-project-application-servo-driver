#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/uaccess.h>

#include "log.h"

MODULE_AUTHOR("Jack Center");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Example use of hardware PWM with platform driver.");

#define DEVICE_NAME "servo_driver"

static struct pwm_device *pwm0;
static u32 pwm_period_ns = 20000000;
static u32 pwm_duty_cycle_ns = 1500000;

static int servo_driver_open(struct inode *inode, struct file *file_p);
static int servo_driver_release(struct inode *inode, struct file *file_p);
static ssize_t servo_driver_read(struct file *file_p, char __user *buffer,
                                 size_t len, loff_t *offset);
static ssize_t servo_driver_write(struct file *file_p,
                                  const char __user *buffer, size_t len,
                                  loff_t *offset);

static int major_number;
static struct cdev servo_cdev;

static const struct file_operations servo_driver_fops = {
    .owner = THIS_MODULE,
    .open = servo_driver_open,
    .read = servo_driver_read,
    .release = servo_driver_release,
    .write = servo_driver_write,
};

static int my_pwm_probe(struct platform_device *pdev) {
  dev_t dev;
  int result;

  LOG_DEBUG("my_pwm_probe");

  result = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
  if (result < 0) {
    LOG_ERROR("Failed to allocate char device region");
    return result;
  }

  major_number = MAJOR(dev);

  cdev_init(&servo_cdev, &servo_driver_fops);
  result = cdev_add(&servo_cdev, dev, 1);
  if (result < 0) {
    LOG_ERROR("Failed to add cdev");
    unregister_chrdev_region(dev, 1);
    return result;
  }

  pwm0 = devm_pwm_get(&pdev->dev, "pwm0");
  if (IS_ERR(pwm0)) {
    int err = PTR_ERR(pwm0);
    if (err == -EPROBE_DEFER) {
      LOG_WARN("PWM not ready, deferring probe");
      return -EPROBE_DEFER;
    }
    LOG_ERROR("Failed to get PWM: %d", err);
    return err;
  }

  pwm_config(pwm0, pwm_duty_cycle_ns, pwm_period_ns);
  pwm_enable(pwm0);

  return 0;
}

static int my_pwm_remove(struct platform_device *pdev) {
  dev_t dev = MKDEV(major_number, 0);

  LOG_DEBUG("my_pwm_remove");
  pwm_disable(pwm0);
  cdev_del(&servo_cdev);
  unregister_chrdev_region(dev, 1);

  return 0;
}

static const struct of_device_id my_pwm_dt_ids[] = {
    {.compatible = "mycompany,my-pwm-device"}, {/* sentinel */}};
MODULE_DEVICE_TABLE(of, my_pwm_dt_ids);

static struct platform_driver my_pwm_driver = {
    .probe = my_pwm_probe,
    .remove = my_pwm_remove,
    .driver =
        {
            .name = DEVICE_NAME,
            .of_match_table = my_pwm_dt_ids,
        },
};

module_platform_driver(my_pwm_driver);

static int servo_driver_open(struct inode *inode, struct file *file_p) {
  LOG_DEBUG("servo_driver_open");
  return 0;
}

static int servo_driver_release(struct inode *inode, struct file *file_p) {
  LOG_DEBUG("servo_driver_release");
  return 0;
}

static ssize_t servo_driver_read(struct file *file_p, char __user *buffer,
                                 size_t len, loff_t *offset) {
  char buf[16];
  int len_written;

  if (*offset > 0)
    return 0;

  len_written = snprintf(buf, sizeof(buf), "%u\n", pwm_duty_cycle_ns);
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

  buf[len] = '\0';

  if (strncmp(buf, "0", 1) == 0) {
    pwm_duty_cycle_ns = 0;
  } else if (strncmp(buf, "1", 1) == 0) {
    pwm_duty_cycle_ns = 5000000;
  } else {
    LOG_WARN("Unknown write value");
    return -EINVAL;
  }

  pwm_config(pwm0, pwm_duty_cycle_ns, pwm_period_ns);
  return len;
}
