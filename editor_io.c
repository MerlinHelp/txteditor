#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "editor_io.h"
#include "global.h"
#include "terminal.h"

int printKeysMode = 0;
int currEditingMode = VIEW;

/*** INPUT ***/

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

void editor_view_mode(char c)
{
    if (printKeysMode == 1) {
        if (iscntrl(c)) {
            (void)printf("%d\r\n", c);
        } else {
            (void)printf("%d ('%c')\r\n", c, c);
        }
    }

    switch (c) {
        /*** CTRL CMDS ***/
        case CTRL_KEY('q'):
            editor_refresh_screen();
            editor_move_cursor_to_top();
            exit(0);
            break;
        case CTRL_KEY('r'):
            editor_refresh_screen();
            editor_move_cursor_to_top();
            break;
        case CTRL_KEY('p'):
            printKeysMode ^= 0x01;
            editor_refresh_screen();
            editor_move_cursor_to_top();
            break;
        case CTRL_KEY('e'):
            currEditingMode ^= 0x01;
            break;
        case CTRL_KEY('c'):
            (void)print_cursor_position();
            break;

        /*** MOVING ***/
        case ENTER:
            editor_move_cursor_next_line();
            break;
        case LOWER_CASE_W:
            editor_move_cursor_up();
            break;
        case LOWER_CASE_A:
            editor_move_cursor_left();
            break;
        case LOWER_CASE_S:
            editor_move_cursor_down();
            break;
        case LOWER_CASE_D:
            editor_move_cursor_right();
            break;
        default:
            break;
    }
}

void editor_edit_mode(char c)
{
    if (printKeysMode == 0 && !iscntrl(c)) {
        (void)write(STDOUT_FILENO, &c, 1);
    }

    switch (c) {
        /*** CTRL CMDS ***/
        case CTRL_KEY('q'):
            editor_refresh_screen();
            editor_move_cursor_to_top();
            exit(0);
            break;
        case CTRL_KEY('r'):
            editor_refresh_screen();
            editor_move_cursor_to_top();
            break;
        case CTRL_KEY('p'):
            printKeysMode ^= 0x01;
            editor_refresh_screen();
            editor_move_cursor_to_top();
            break;
        case CTRL_KEY('e'):
            currEditingMode ^= 0x01;
            break;

        // TODO MAP ARROW KEYS WHEN IN EDITING MODE
        /*** MOVING ***/
        /* case LOWER_CASE_W:
            editor_move_cursor_up();
            break;
        case LOWER_CASE_A:
            editor_move_cursor_left();
            break;
        case LOWER_CASE_S:
            editor_move_cursor_down();
            break;
        case LOWER_CASE_D:
            editor_move_cursor_right();
            break;
        default:
            break;
        */
    }
}

int editor_process_keypress(void)
{
    char c = editor_read_keypress();
    
    switch (currEditingMode) {
        case VIEW:
            editor_view_mode(c);
            break;
        case EDIT:
            editor_edit_mode(c);
            break;
        default:
            die("non-existent editingMode, error in function editor_process_keypress");
            break;            
    }
    
    return 0;
}

/*** OUTPUT ***/

int print_cursor_position(void)
{
    int *size = malloc(sizeof(*size));
    int *row = malloc(sizeof(*row));
    if (row == NULL) {
        free(row);
        return -1;
    }
    int *col = malloc(sizeof(*col));
    if (col == NULL) {
        free(col);
        return -1;
    }

    get_cursor_position(row, col, size);

    char buf[*size];
    sprintf(buf, "%d,%d", *row, *col);

    // printf("%s\r\n", buf);
    if (write(STDOUT_FILENO, buf, *size - 1) != *size - 1) {
        free(row);
        free(col);
        die("write, error in function print_cursor_function");
    }

    free(row);
    free(col);
    return 0;
}

int editor_refresh_screen(void)
{
    errno = 0;

    // Clears everything
    if (write(STDOUT_FILENO, "\x1b[2J", 4) == -1 && errno != 0) {
        die("write, error in function editor_refresh_screen");
    }

    return 0;
}

int editor_draw_empty_rows()
{
    errno = 0;

    for (int i = 0; i < EC.screenrows - 1; ++i) {
        if (write(STDOUT_FILENO, "~\r\n", 3) == -1 && errno != 0) {
            die("write, error in function editor_draw_empty_rows"); 
        }
    }

    if (write(STDOUT_FILENO, "~\r", 3) == -1 && errno != 0) {
        die("write, error in function editor_draw_empty_rows"); 
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

    char tmpStr[7]; 
    sprintf(tmpStr, "\x1b[%d;%dH", row, col);

    errno = 0;
    // Moves cursor to row argument, col argument
    if (write(STDOUT_FILENO, tmpStr, 6) != 6 && errno != 0) {
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
    if (write(STDOUT_FILENO, "\x1b[E", 3) != 3 && errno != 0) {
        die("write, error in function editor_move_cursor_next_line");
    }
    
    return 0;
    
}

int editor_move_cursor_up(void)
{
    errno = 0;
    // Moves cursor up
    if (write(STDOUT_FILENO, "\x1b[A", 3) != 3 && errno != 0) {
        die("write, error in function editor_move_cursor_up");
    }
    
    return 0;
    
}

int editor_move_cursor_left(void)
{
    errno = 0;
    // Move cursor left
    if (write(STDOUT_FILENO, "\x1b[D", 3) != 3 && errno != 0) {
        die("write, error in function editor_move_cursor_left");
    }
    
    return 0;
    
}

int editor_move_cursor_down(void)
{
    errno = 0;
    // Move cursor down
    if (write(STDOUT_FILENO, "\x1b[B", 3) != 3 && errno != 0) {
        die("write, error in function editor_move_cursor_down");
    }
    
    return 0;
    
}

int editor_move_cursor_right(void)
{
    errno = 0;
    // Move cursor right
    if (write(STDOUT_FILENO, "\x1b[C", 3) != 3 && errno != 0) {
        die("write, error in function editor_move_cursor_right");
    }
    
    return 0;
    
}
