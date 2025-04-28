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

  Servo servo_0 = servo_create("/dev/servo_driver_0");
  Servo servo_1 = servo_create("/dev/servo_driver_1");
  if (!servo_init(&servo_0)) {
    printf("Error: failed to open servo: %s\r\n", servo_0.handle);
    return 1;
  }

  if (!servo_init(&servo_1)) {
    printf("Error: failed to open servo: %s\r\n", servo_1.handle);
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
      if (seq[0] == 'q' || seq[0] == 'Q') {
        if (!servo_disable(&servo_0)) {
          printf("Error: failed to disable servo: %s\r\n", servo_0.handle);
        }

        if (!servo_disable(&servo_1)) {
          printf("Error: failed to disable servo: %s\r\n", servo_1.handle);
        }
        break;
      }
      
      if (seq[0] == '\x1b') { // Escape sequence for arrows
        if (read(STDIN_FILENO, &seq[1], 1) == 1 && seq[1] == '[') {
          if (read(STDIN_FILENO, &seq[2], 1) == 1) {
            switch (seq[2]) {
            case 'A':
              printf("Up Arrow (pressed)\n");
              if (!servo_set_position(180, &servo_1)) {
                printf("Error: failed to set poistion on servo: %s\r\n", servo_1.handle);
              }
              break;
            case 'B':
              printf("Down Arrow (pressed)\n");
              if (!servo_set_position(0, &servo_1)) {
                printf("Error: failed to set poistion on servo: %s\r\n", servo_1.handle);
              }
              break;
            case 'C': {
              printf("Right Arrow (pressed)\n");
              if (!servo_set_position(0, &servo_0)) {
                printf("Error: failed to set poistion on servo: %s\r\n", servo_0.handle);
              }
              break;
            }
            case 'D': {
              printf("Left Arrow (pressed)\n");
              if (!servo_set_position(180, &servo_0)) {
                printf("Error: failed to set poistion on servo: %s\r\n", servo_0.handle);
              }
              break;
            }
            }
          }
        }
      } else {
        switch (tolower(seq[0])) {
        case 'i':
          printf("i / Enable (pressed)\n");
          if (!servo_enable(&servo_0)) {
            printf("Error: failed to enable servo: %s\r\n", servo_0.handle);
          }

          if (!servo_enable(&servo_1)) {
            printf("Error: failed to enable servo: %s\r\n", servo_1.handle);
          }
        case 'o':
          printf("o / Disable (pressed)\n");
          if (!servo_disable(&servo_0)) {
            printf("Error: failed to disable servo: %s\r\n", servo_0.handle);
          }

          if (!servo_disable(&servo_1)) {
            printf("Error: failed to disable servo: %s\r\n", servo_1.handle);
          }
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
