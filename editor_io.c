#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "global.h"

/*** INPUT ***/

char editor_read_keypress(void)
{
    int nread;
    char c;

    // Until 1 byte returned from read() unistd func, keep checking for input
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            die("read, error in function editor_read_char");
        }
    }

    return c;
}

void editor_process_keypress(void)
{
    char c = editor_read_keypress();
    
    if (iscntrl(c)) {
        printf("%d\r\n", c);
    } else {
        printf("%d ('%c')\r\n", c, c);
    }

    switch (c) {
        case CTRL_KEY('q'):
            exit(0);
            break;
        default:
            break;
    }
}

/*** OUTPUT ***/
