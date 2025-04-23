#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

int main() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    printf("Listening for keypresses (q to quit)...\n");

    while (1) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);

        struct timeval timeout = {1, 0}; // 1 second timeout

        int res = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);
        if (res > 0) {
            char c = getchar();
            printf("Pressed: %c\n", c);
            if (c == 'q') break;
        } else {
            // Timeout hit, do background work if needed
            printf(".");
            fflush(stdout);
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return 0;
}
