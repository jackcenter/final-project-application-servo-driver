#include "servo.h"

#include <fcntl.h>
#include <linux/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "servo_ioctl.h"

bool servo_close(const Servo *const servo) {
  if (close(servo->fd)) {
    perror("close");
    return false;
  }
  return true;
}

Servo servo_create(const char *const handle) {
  Servo servo;
  servo.fd = -1;

  if (handle == NULL) {
    printf("Error: handle is NULL\r\n");
    strncpy(servo.handle, "UNCONFIGURED\0", sizeof(servo.handle));
    return servo;
  }

  strncpy(servo.handle, handle, sizeof(servo.handle) - 1);
  servo.handle[sizeof(servo.handle) - 1] = '\0';
  return servo;
}

bool servo_disable(const Servo *const servo) {
  if (servo == NULL) {
    printf("Error: servo is NULL\r\n");
    return false;
  }

  if (servo->fd < 0) {
    printf("Error: servo not properly initialized\r\n");
    return false;
  }

  if (ioctl(servo->fd, SERVO_DISABLE) < 0) {
    printf("Error: failed to disable servo\r\n");
    return false;
  }
  return true;
}

bool servo_enable(const Servo *const servo) {
  if (servo == NULL) {
    printf("Error: servo is NULL\r\n");
    return false;
  }

  if (servo->fd < 0) {
    perror("servo_enable");
    return false;
  }

  if (ioctl(servo->fd, SERVO_ENABLE) < 0) {
    perror("ioctl");
    return false;
  }
  return true;
}

int servo_get_position(const Servo *const servo) {
  if (servo == NULL) {
    printf("Error: servo is NULL\r\n");
    return -1;
  }

  if (servo->fd < 0) {
    printf("Error: servo not properly initialized\r\n");
    return -1;
  }

  char data[12];
  ssize_t bytes_read = read(servo->fd, data, sizeof(data) - 1);
  if (bytes_read == -1) {
    perror("read");
    return -1;
  }

  data[bytes_read] = '\0';
  int pos = -1;
  if (sscanf(data, "%d", &pos) != 1) {
    printf("Error: sscanf\r\n");
    return -1;
  }
  return pos;
}

bool servo_init(Servo *const servo) {
  if (servo == NULL) {
    printf("Error: servo is NULL\r\n");
    return false;
  }

  servo->fd = open(servo->handle, O_WRONLY);
  if (servo->fd < 0) {
    perror("open");
    return false;
  }
  return true;
}

bool servo_set_position(const int pos, const Servo *const servo) {
  if (servo == NULL) {
    printf("Error: servo is NULL\r\n");
    return false;
  }

  if (servo->fd < 0) {
    printf("Error: servo not properly initialized\r\n");
    return false;
  }

  char data[12];
  snprintf(data, sizeof(data), "%d", pos);
  if (write(servo->fd, data, strlen(data)) < 0) {
    perror("write");
    return false;
  }

  return true;
}
