#include <ctype.h>
#include <stdio.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

int main() {
  struct termios oldt, newt;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  int fd = open("/dev/servo_driver", O_WRONLY);
  if (fd < 0) {
    perror("Failed to open /dev/servo_driver");
    return 1;
  }

  printf("Listening for arrow keys and WASD (press 'q' to quit)...\n");

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
              break;
            case 'B':
              printf("Down Arrow (pressed)\n");
              break;
            case 'C': {
              printf("Right Arrow (pressed)\n");
              const char *data = "0"; // e.g., 75% duty cycle
              if (write(fd, data, strlen(data)) < 0) {
                perror("Failed to write to servo driver");
                close(fd);
                return 1;
              }
              break;
            }
            case 'D': {
              printf("Left Arrow (pressed)\n");
              const char *data = "180"; // e.g., 75% duty cycle
              if (write(fd, data, strlen(data)) < 0) {
                perror("Failed to write to servo driver");
                close(fd);
                return 1;
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
