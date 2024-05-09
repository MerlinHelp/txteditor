#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "editor_io.h"
#include "global.h"

/*** INPUT ***/
int printkeys = 0;

char editor_read_keypress(void)
{
    int nread;
    char c;

    errno = 0;
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
    
    if (printkeys == 1) {
        if (iscntrl(c)) {
            (void)printf("%d\r\n", c);
        } else {
            (void)printf("%d ('%c')\r\n", c, c);
        }
    } else if (!iscntrl(c)) {
        (void)write(STDOUT_FILENO, &c, 1);
    }

    switch (c) {
        case CTRL_KEY('q'):
            exit(0);
            break;
        case CTRL_KEY('r'):
            editor_refresh_screen();
            editor_move_cursor_to_top();
            break;
        case CTRL_KEY('p'):
            printkeys ^= 0x01;
            editor_refresh_screen();
            editor_move_cursor_to_top();
            break;
        case ENTER:
            editor_move_cursor_next_line();
            break;
        default:
            break;
    }
}

/*** OUTPUT ***/

int editor_refresh_screen(void)
{
    errno = 0;

    // Clears everything
    if (write(STDOUT_FILENO, "\x1b[2J", 4) == -1 && errno != 0) {
        die("write, error in function editor_refresh_screen");
    }

    return 0;
}

int editor_move_cursor(int row, int col)
{
    if (row > 100) {
        row = 100;
    } else if (row < 1) {
        row = 1;
    }
    if (col > 100) {
        col = 100;
    } else if (col < 1) {
        col = 1;
    }

    char tmpstr[7]; 
    sprintf(tmpstr, "\x1b[%d;%dH", row, col);

    errno = 0;
    // Moves cursor to row argument, col argument
    if (write(STDOUT_FILENO, tmpstr, 6) == -1 && errno != 0) {
        die("write, error in function editor_move_cursor");
    }
    
    return 0;
}

int editor_move_cursor_to_top(void)
{
    return editor_move_cursor(1, 1);
}

int editor_move_cursor_next_line(void)
{
    errno = 0;
    // Move cursor down and to the beginning of a line
    if (write(STDOUT_FILENO, "\x1b[E", 3) == -1 && errno != 0) {
        die("write, error in function editor_move_cursor_next_line");
    }
    
    return 0;
    
}

int editor_move_cursor_up(void)
{
    errno = 0;
    // Moves cursor up
    if (write(STDOUT_FILENO, "\x1b[A", 3) == -1 && errno != 0) {
        die("write, error in function editor_move_cursor_up");
    }
    
    return 0;
    
}
int editor_move_cursor_left(void)
{
    errno = 0;
    // Move cursor left
    if (write(STDOUT_FILENO, "\x1b[D", 3) == -1 && errno != 0) {
        die("write, error in function editor_move_cursor_left");
    }
    
    return 0;
    
}
int editor_move_cursor_down(void)
{
    errno = 0;
    // Move cursor down
    if (write(STDOUT_FILENO, "\x1b[B", 3) == -1 && errno != 0) {
        die("write, error in function editor_move_cursor_down");
    }
    
    return 0;
    
}
int editor_move_cursor_right(void)
{
    errno = 0;
    // Move cursor right
    if (write(STDOUT_FILENO, "\x1b[C", 3) == -1 && errno != 0) {
        die("write, error in function editor_move_cursor_right");
    }
    
    return 0;
    
}
