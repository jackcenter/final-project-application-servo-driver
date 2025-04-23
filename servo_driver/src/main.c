#include <ctype.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include "servo.h"
#include "servo_ioctl.h"

int main() {
  struct termios oldt, newt;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  Servo servo = servo_create("/dev/servo_driver");
  if (!servo_init(&servo)) {
    printf("Error: failed to open servo: %s\r\n", servo.handle);
    return 1;
  }

  printf("Listening for arrow keys and WASD (press 'q' to quit)...\r\n");

  while (1) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    select(STDIN_FILENO + 1, &set, NULL, NULL, NULL);

    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) == 1) {
      if (seq[0] == 'q' || seq[0] == 'Q')
        break;

      if (seq[0] == '\x1b') { // Escape sequence for arrows
        if (read(STDIN_FILENO, &seq[1], 1) == 1 && seq[1] == '[') {
          if (read(STDIN_FILENO, &seq[2], 1) == 1) {
            switch (seq[2]) {
            case 'A':
              printf("Up Arrow (pressed)\n");
              if (!servo_enable(&servo)) {
                printf("Error: failed to enable servo: %s\r\n", servo.handle);
              }
              break;
            case 'B':
              printf("Down Arrow (pressed)\n");
              if (!servo_disable(&servo)) {
                printf("Error: failed to disable servo: %s\r\n", servo.handle);
              }
              break;
            case 'C': {
              printf("Right Arrow (pressed)\n");
              if (!servo_set_position(0, &servo)) {
                printf("Error: failed to set poistion on servo: %s\r\n", servo.handle);
              }
              break;
            }
            case 'D': {
              printf("Left Arrow (pressed)\n");
              if (!servo_set_position(180, &servo)) {
                printf("Error: failed to set poistion on servo: %s\r\n", servo.handle);
              }
              break;
            }
            }
          }
        }
      } else {
        switch (tolower(seq[0])) {
        case 'w':
          printf("W / Up (pressed)\n");
          break;
        case 'a':
          printf("A / Left (pressed)\n");
          break;
        case 's':
          printf("S / Down (pressed)\n");
          break;
        case 'd':
          printf("D / Right (pressed)\n");
          break;
        default:
          printf("Pressed: %c\n", seq[0]);
          break;
        }
      }
    }
  }

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return 0;
}
