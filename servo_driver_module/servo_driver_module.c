#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/types.h>

#include "log.h"

#define IO_LED 21

#define IO_OFFSET 0

static int servo_driver_major = 0; // use dynamic major
static int servo_driver_minor = 0;

static struct cdev servo_driver_cdev;

static struct pwm_device *pwm;
static struct pwm_state state;
struct platform_device *pwm_dummy_dev;

MODULE_AUTHOR("Jack Center");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Example use the hardware PWM.");

struct pwm_device *pwm0 = NULL;
u32 pwm_period_ns = 20000000;
u32 pwm_duty_cycle_ns = 0;

struct pwm_device *request_rpi_pwm(int chip_index, int pwm_index);

static void servo_driver_exit(void);

static int servo_driver_init(void);

static int servo_driver_open(struct inode *inode, struct file *file_p);

static ssize_t servo_driver_read(struct file *file_p, char __user *buffer,
                                 size_t len, loff_t *offset);

static int servo_driver_release(struct inode *inode, struct file *file_p);

static ssize_t servo_driver_write(struct file *file_p,
                                  const char __user *buffer, size_t len,
                                  loff_t *offset);

module_init(servo_driver_init);
module_exit(servo_driver_exit);

struct file_operations servo_driver_fops = {
    .open = servo_driver_open,
    .read = servo_driver_read,
    .release = servo_driver_release,
    .write = servo_driver_write,
};

static void servo_driver_exit(void) {
  LOG_DEBUG("servo_driver_exit");

  pwm_disable(pwm0);
  platform_device_unregister(pwm_dummy_dev);
  cdev_del(&servo_driver_cdev);

  dev_t dev = MKDEV(servo_driver_major, servo_driver_minor);
  unregister_chrdev_region(dev, 1);
}

static int servo_driver_init(void) {
  LOG_DEBUG("servo_driver_init");
  dev_t dev = 0;

  int result = alloc_chrdev_region(&dev, servo_driver_minor, 1, "servo_driver");
  if (result < 0) {
    LOG_ERROR("servo_driver failed to allocate a major number");
    return result;
  }

  servo_driver_major = MAJOR(dev);
  LOG_DEBUG("servo_driver registered with major number %d", servo_driver_major);

  cdev_init(&servo_driver_cdev, &servo_driver_fops);
  servo_driver_cdev.owner = THIS_MODULE;
  servo_driver_cdev.ops = &servo_driver_fops;

  result = cdev_add(&servo_driver_cdev, dev, 1);
  if (result < 0) {
    LOG_ERROR("failed to add cdev");
    unregister_chrdev_region(dev, 1);
    return result;
  }

  // Get the PWM device associated with pwmchip0 (if pwmchip0 exists)
  struct platform_device *pdev = platform_device_register_simple("pwm", -1, NULL, 0);
  if (IS_ERR(pdev)) {
      LOG_ERROR("Failed to register platform device\n");
      return PTR_ERR(pdev);
  }

  pwm = pwm_get(&pdev->dev, "pwm0");
  if (IS_ERR(pwm)) {
    LOG_ERROR("Failed to get PWM device");
    platform_device_unregister(pdev);
    cdev_del(&servo_driver_cdev);
    unregister_chrdev_region(dev, 1);
    return PTR_ERR(pwm);
  }

  pwm_get_state(pwm, &state);
  state.period = 20000000;
  state.duty_cycle = 1500000;
  state.enabled = true;
  pwm_apply_state(pwm, &state);

  return 0;
}

static int servo_driver_open(struct inode *inode, struct file *file_p) {
  LOG_DEBUG("servo_driver_open");
  return 0;
}

static ssize_t servo_driver_read(struct file *file_p, char __user *buffer,
                                 size_t len, loff_t *offset) {
  LOG_DEBUG("servo_driver_read");
  if (*offset > 0) {
    return 0;
  }

  char pin_value_str[16];
  int pin_value_str_len =
      snprintf(pin_value_str, sizeof(pin_value_str), "%u\n", pwm_duty_cycle_ns);
  const size_t bytes_not_written =
      copy_to_user(buffer, pin_value_str, pin_value_str_len);
  LOG_DEBUG("copy_to_user returned %lu", bytes_not_written);

  const size_t bytes_written = pin_value_str_len - bytes_not_written;
  *offset += bytes_written;
  return bytes_written;
}

static int servo_driver_release(struct inode *inode, struct file *file_p) {
  LOG_DEBUG("servo_driver_release");
  return 0;
}

static ssize_t servo_driver_write(struct file *file_p,
                                  const char __user *buffer, size_t len,
                                  loff_t *offset) {
  LOG_DEBUG("servo_driver_write");
  if (*offset > 0) {
    return 0;
  }

  char user_str[16];
  size_t copy_len = min(len, sizeof(user_str) - 1);
  const size_t retval = copy_from_user(user_str, buffer, copy_len);
  if (retval != 0) {
    LOG_ERROR("copy_from_user returned %ld", retval);
    return -EFAULT;
  }

  user_str[copy_len] = '\0';
  *offset += copy_len;

  char *newline_char_p = strchr(user_str, '\n');
  if (newline_char_p) {
    *newline_char_p = '\0';
  }

  if (strcmp(user_str, "0") == 0) {
    LOG_DEBUG("Write: OFF");
    pwm_duty_cycle_ns = 0;
  } else if (strcmp(user_str, "1") == 0) {
    LOG_DEBUG("Write: ON");
    pwm_duty_cycle_ns = 5000000;
  } else {
    LOG_WARN("Unknown write command. Acceptable commands are `0` or `1`");
  }

  pwm_config(pwm0, pwm_duty_cycle_ns, pwm_period_ns);

  return copy_len;
}
