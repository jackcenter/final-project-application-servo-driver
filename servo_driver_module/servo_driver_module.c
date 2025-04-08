#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>

#include "log.h"

static int servo_driver_major = 0; // use dynamic major
static int servo_driver_minor = 0;

static struct cdev servo_driver_cdev;

MODULE_AUTHOR("Jack Center");
MODULE_LICENSE("Dual BSD/GPL");

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

  cdev_del(&servo_driver_cdev);

  dev_t dev = MKDEV(servo_driver_major, servo_driver_minor);
  unregister_chrdev_region(dev, 1);
}

static int servo_driver_init(void) {
  LOG_DEBUG("servo_driver_init");
  dev_t dev = 0;

  int result = alloc_chrdev_region(&dev, servo_driver_minor, 1, "servo_driver");
  if (result < 0) {
    LOG_WARN("servo_driver failed to allocate a major number");
    return result;
  }

  servo_driver_major = MAJOR(dev);
  LOG_DEBUG("servo_driver registered with major number %d", servo_driver_major);

  cdev_init(&servo_driver_cdev, &servo_driver_fops);
  servo_driver_cdev.owner = THIS_MODULE;
  servo_driver_cdev.ops = &servo_driver_fops;

  result = cdev_add(&servo_driver_cdev, dev, 1);
  if (result < 0) {
    LOG_WARN("failed to add cdev");
    unregister_chrdev_region(dev, 1);
    return result;
  }

  return 0;
}

static int servo_driver_open(struct inode *inode, struct file *file_p) {
  LOG_DEBUG("servo_driver_open");
  return 0;
}

static ssize_t servo_driver_read(struct file *file_p, char __user *buffer,
                                 size_t len, loff_t *offset) {
  LOG_DEBUG("servo_driver_read");
  return 0; 
}

static int servo_driver_release(struct inode *inode, struct file *file_p) {
  LOG_DEBUG("servo_driver_release");
  return 0;
}

static ssize_t servo_driver_write(struct file *file_p,
                                  const char __user *buffer, size_t len,
                                  loff_t *offset) {
  LOG_DEBUG("servo_driver_write");
  return len;
}
