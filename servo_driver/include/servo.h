#ifndef SERVO_H
#define SERVO_H

#include <stdbool.h>

typedef struct {
  char handle[64];
  int fd;
} Servo;

bool servo_close(const Servo *servo);

Servo servo_create(const char *const handle);

bool servo_disable(const Servo *servo);

bool servo_enable(const Servo *servo);

int servo_get_position(const Servo *servo);

bool servo_init(Servo *const servo);

bool servo_set_position(const int pos, const Servo *servo);

#endif // SERVO_H
