#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <ctype.h>

int main() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    printf("Listening for arrow keys and WASD (press 'q' to quit)...\n");

    while (1) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);

        select(STDIN_FILENO + 1, &set, NULL, NULL, NULL);

        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) == 1) {
            if (seq[0] == 'q' || seq[0] == 'Q') break;

            if (seq[0] == '\x1b') { // Escape sequence for arrows
                if (read(STDIN_FILENO, &seq[1], 1) == 1 && seq[1] == '[') {
                    if (read(STDIN_FILENO, &seq[2], 1) == 1) {
                        switch (seq[2]) {
                            case 'A': printf("Up Arrow (pressed)\n"); break;
                            case 'B': printf("Down Arrow (pressed)\n"); break;
                            case 'C': printf("Right Arrow (pressed)\n"); break;
                            case 'D': printf("Left Arrow (pressed)\n"); break;
                        }
                    }
                }
            } else {
                switch (tolower(seq[0])) {
                    case 'w': printf("W / Up (pressed)\n"); break;
                    case 'a': printf("A / Left (pressed)\n"); break;
                    case 's': printf("S / Down (pressed)\n"); break;
                    case 'd': printf("D / Right (pressed)\n"); break;
                    default: printf("Pressed: %c\n", seq[0]); break;
                }
            }
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return 0;
}
