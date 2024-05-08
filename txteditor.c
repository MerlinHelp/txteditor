#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include "rawmode.h"
#include "txteditor.h"

int main(void)
{
    enable_raw_mode();

    while (1) {
        char c = '\0';
        if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) {
            die("read, error in function main");
        }

        if (c == 0) {
            continue;
        }

        if (iscntrl(c)) {
            printf("%d\r\n", c);
        } else {
            printf("%d ('%c')\r\n", c, c);
        }

        if (c == CTRL_KEY('q')) {
            break;
        }
    }

    return EXIT_SUCCESS;
}
