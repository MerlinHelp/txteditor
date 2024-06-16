#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "editor_io.h"
#include "global.h"
#include "terminal.h"

#define CHAD_VERSION "0.0.1"

// TODO: Make macros for printKeysMode OR combine the modes together
int printKeysMode = 0;
int currEditingMode = VIEW;
typedef struct abuf abuf;

/*** INPUT ***/

int editor_read_keypress(void)
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

    if (c == '\x1b') {
        char seq[3];
            
        if (read(STDIN_FILENO, &seq[0], 1) != 1) {
            return '\x1b';
        }
        if (read(STDIN_FILENO, &seq[1], 1) != 1) {
            return '\x1b';
        }

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A':
                    return ARROW_UP;
                case 'B':
                    return ARROW_DOWN;
                case 'C':
                    return ARROW_RIGHT;
                case 'D':
                    return ARROW_LEFT;
            }
        }

        return '\x1b';
    }

    return c;
}

int editor_process_cursor_movement(int key)
{
    switch (key) {
        case LOWER_CASE_W:
        case ARROW_UP:
            if (EC.csrY != 0) {
                --EC.csrY;
            }
            break;
        case LOWER_CASE_A:
        case ARROW_LEFT:
            if (EC.csrX != 0) {
                --EC.csrX;
            }
            break;
        case LOWER_CASE_S:
        case ARROW_DOWN:
            if (EC.csrY != EC.screenRows - 1) {
                ++EC.csrY;
            }
            break;
        case LOWER_CASE_D:
        case ARROW_RIGHT:
            if (EC.csrX != EC.screenCols - 1) {
                ++EC.csrX;
            }
            break;
    }
    return EXIT_SUCCESS;
}

void editor_view_mode(int c)
{
    if (printKeysMode == 1) {
        if (iscntrl(c)) {
            (void)printf("%d\r\n", c);
        } else {
            (void)printf("%d ('%c')\r\n", c, c);
        }

        if (c > 31) {
            return;
        }
    }

    switch (c) {
        /*** CTRL CMDS ***/
        case CTRL_KEY('q'):
            editor_reset_screen();
            exit(0);
            break;
        case CTRL_KEY('r'):
            editor_refresh_screen();
            break;
        case CTRL_KEY('p'):
            printKeysMode ^= 0x01;
            if (printKeysMode == 1) {
                editor_reset_screen();
            } else {
                editor_refresh_screen();
            }
            break;
        case CTRL_KEY('e'):
            currEditingMode ^= 0x01;
            if (printKeysMode == 0) {
                editor_refresh_screen();
            }
            printKeysMode = 0;
            break;
        case CTRL_KEY('c'):
            (void)print_cursor_position();
            break;

        /*** MOVING ***/
        case LOWER_CASE_W:
        case ARROW_UP:
        case LOWER_CASE_A:
        case ARROW_LEFT:
        case LOWER_CASE_S:
        case ARROW_DOWN:
        case LOWER_CASE_D:
        case ARROW_RIGHT:
            (void)editor_process_cursor_movement(c);
            break;
        default:
            break;
    }
}

void editor_edit_mode(int c)
{
    if (!iscntrl(c)) {
        (void)write(STDOUT_FILENO, &c, 1);
    }

    switch (c) {
        /*** CTRL CMDS ***/
        case CTRL_KEY('q'):
            editor_reset_screen();
            exit(0);
            break;
        case CTRL_KEY('r'):
            editor_refresh_screen();
            break;
        case CTRL_KEY('e'):
            currEditingMode ^= 0x01;
            printKeysMode = 0;
            break;

        // TODO MAP ARROW KEYS WHEN IN EDITING MODE
        /*** MOVING ***/
        case ARROW_UP:
        case ARROW_LEFT:
        case ARROW_DOWN:
        case ARROW_RIGHT:
            editor_process_cursor_movement(c);
            break;
        default:
            break;
    }
}

int editor_process_keypress(void)
{
    int c = editor_read_keypress();
    
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


void ab_append(abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);
  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void ab_free(abuf *ab) {
  free(ab->b);
}

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

int editor_reset_screen(void)
{
    abuf ab = ABUF_INIT;

    ab_append(&ab, "\x1b[2J", 4);
    ab_append(&ab, "\x1b[H", 3);

    errno = 0;
    if (write(STDOUT_FILENO, ab.b, ab.len) != ab.len && errno != 0) {
        die("write, error in editor_reset_screen");
    }

    ab_free(&ab);

    return 0;
}

int editor_refresh_screen(void)
{

    // Clears everything
    abuf ab = ABUF_INIT;

    ab_append(&ab, "\x1b[?25l", 6);
    ab_append(&ab, "\x1b[H", 3);

    editor_draw_empty_rows(&ab);

    editor_move_cursor(&ab, EC.csrY + 1, EC.csrX + 1);
    ab_append(&ab, "\x1b[?25h", 6);
    // char buf[32];
    // snprintf(buf, sizeof(buf), "%d,%d", EC.csrY + 1, EC.csrX + 1);
    // ab_append(&ab, buf, strlen(buf);

    errno = 0;
    if (write(STDOUT_FILENO, ab.b, ab.len) != ab.len && errno != 0) {
        die("write, error in editor_refresh_screen");
    }

    ab_free(&ab);


    return 0;
}

int editor_draw_empty_rows(abuf *ab)
{
    // Draw empty rows
    for (int i = 0; i < EC.screenRows - 1; ++i) {
        ab_append(ab, "~\x1b[K\r\n", 6);
    }

    // Draw welcome message
    char welcome[80];
    int welcomelen = snprintf(welcome, sizeof(welcome),
        "Chad Editor -- version %s", CHAD_VERSION);
    if (welcomelen > EC.screenCols) {
        welcomelen = EC.screenCols;
    }

    int padding = (EC.screenCols - welcomelen) / 2;
    while (padding--) {
        ab_append(ab, " ", 1);
    }

    ab_append(ab, welcome, welcomelen);
    ab_append(ab, "\x1b[K", 3);

    return 0;
}

int editor_move_cursor(abuf *ab, int row, int col)
{
    size_t nbytes = snprintf(NULL, 0, "\x1b[%d;%dH", row, col) + 1;
    char buf[nbytes];
    snprintf(buf, nbytes, "\x1b[%d;%dH", row, col);

    // Moves cursor to row argument, col argument
    ab_append(ab, buf, nbytes);
    
    return 0;
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
