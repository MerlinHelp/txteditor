#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "global.h"
#include "editor_io.h"
#include "terminal.h"

#define CHAD_VERSION "0.0.1"
#define LAST_ROW_OFF 1
#define LAST_COL_OFF 0
#define TAB_STOP 8

// TODO: Make macros for printKeysMode OR combine the modes together
int printKeysMode = 0;
int currEditingMode = VIEW;

/*** FILE_IO ***/

// MUST ONLY BE CALLED IN EDIT MODE (currEditingMode)
void editor_row_insert_char(erow *row, int at, int c)
{
    if (at < 0) {
        at = 0;
    } else if (at > row->size) {
        at = row->size;
    }

    // + 2 to make space for null?, but if null is already gonna be there later
    // on why do we need to add another null...
    errno = 0;
    row->chars = realloc(row->chars, row->size + 2);
    if (row->chars == NULL && errno != 0) {
        die("realloc, error in function editor_row_insert_char");
    }

    errno = 0;
    memmove(&(row->chars[at + 1]), &(row->chars[at]), row->size - at + 1);
    if (row->chars == NULL && errno != 0) {
        die("memmove, error in function editor_row_insert_char");
    }
    
    ++row->size;
    row->chars[at] = c;
    editor_update_row(row);
}

void editor_insert_char(int c)
{
    if (EC.csrY == EC.numRows) {
        editor_append_row("", 0);
    }
    editor_row_insert_char(&EC.rows[EC.csrY], EC.csrX, c);
    ++EC.csrX;
}

void editor_row_delete_char(erow *row, int at)
{
    if (at < 0) {
        return;
    } else if (at > row->size) {
        at = row->size - 1;
    }

    errno = 0;
    memmove(&(row->chars[at]), &(row->chars[at + 1]), row->size - at - 1);
    // TODO: Probably wrong way to ERROR check for memmove, instead look at
    // memmove docs and check for return val.
    if (row->chars == NULL && errno != 0) {
        die("memmove, error in function editor_row_delete_char");
    }

    errno = 0;
    row->chars = realloc(row->chars, row->size - 1);
    if (row->chars == NULL && errno != 0) {
        die("realloc, error in function editor_row_delete_char");
    }

    --row->size;
    editor_update_row(row);
}

// For deletion functions, we will memmove first and then realloc, since we will
// lose the end data if we realloc first
void editor_delete_char()
{
    if (EC.csrY < EC.numRows) {
        if (EC.rows[EC.csrY].size == 0 && EC.csrY > 0) {
            editor_delete_row(EC.csrY);
            editor_process_cursor_movement(ARROW_LEFT);
            return;
        }
        if (EC.csrX > 0) {
            editor_row_delete_char(&EC.rows[EC.csrY], EC.csrX - 1);
        }
    }

    --EC.csrX;
}

void editor_delete_row(int currRow)
{
    errno = 0;
    memmove(&(EC.rows[currRow]), &(EC.rows[currRow + 1]), 
           (sizeof(erow) * (EC.numRows - currRow - 1)));
    if (EC.rows == NULL && errno != 0) {
        die("memmove, error in function editor_delete_row");
    }

    errno = 0;
    EC.rows = realloc(EC.rows, sizeof(erow) * (EC.numRows - 1));

    if (EC.rows == NULL && errno != 0) {
        die("realloc, error in function editor_delete_row");
    }

    --EC.numRows;
}

void editor_update_row(erow *row)
{
    int tabs = 0;
    for (int i = 0; i < row->size; ++i) {
        if ((row->chars)[i] == '\t') {
            ++tabs;
        }
    }
    free(row->render);
    
    if ((row->render = malloc(row->size + (tabs * (TAB_STOP - 1)) + 1)) == NULL) {
        die("malloc, error in function editor_update_row");
    }

    int idx = 0;
    for (int i = 0; i < row->size; ++i) {
        if ((row->chars)[i] == '\t') {
            do {
                (row->render)[idx++] = ' ';
            } while (idx % TAB_STOP != 0);
        } else {
            (row->render)[idx++] = (row->chars)[i];
        }
    }

    row->render[idx] = '\0';
    row->rsize = idx;
}

void editor_append_row(const char *s, size_t len)
{
    errno = 0;
    EC.rows = realloc(EC.rows, sizeof(erow) * (EC.numRows + 1));

    if (EC.rows == NULL && errno != 0) {
        die("realloc, error in function editor_append_row");
    }

    int newIndex = EC.numRows;
    erow *newRow = &EC.rows[newIndex];
    newRow->size = len;
    if ((newRow->chars = malloc(len + 1)) == NULL) {
        die("malloc, error in function editor_append_row");
    }

    memcpy(newRow->chars, s, len);
    (newRow->chars)[len] = '\0';

    newRow->rsize = 0;
    newRow->render = NULL;
    editor_update_row(newRow);

    ++EC.numRows;
}

void editor_open(const char *filename)
{
    free(EC.filename);
    EC.filename = strdup(filename);
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        die("fopen, error in function editor_open");
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&line, &linecap, fp)) != EOF) {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                               line[linelen - 1] == '\r')) {
            --linelen;
        }
        editor_append_row(line, linelen);
    }

    free(line);
    fclose(fp);
    
}


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
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) {
                    return '\x1b';
                }

                if (seq[2] == '~') {
                    switch(seq[1]) {
                        case '1':
                        case '7':
                            return HOME_KEY;
                        case '4':
                        case '8':
                            return END_KEY;
                        case '3':
                            return DEL_KEY;
                        case '5': 
                            return PAGE_UP;
                        case '6':
                            return PAGE_DOWN;
                    }
                }

            } else {
                switch (seq[1]) {
                    case 'A':
                        return ARROW_UP;
                    case 'B':
                        return ARROW_DOWN;
                    case 'C':
                        return ARROW_RIGHT;
                    case 'D':
                        return ARROW_LEFT;
                    case 'H':
                        return HOME_KEY;
                    case 'F':
                        return END_KEY;
                }
            } 
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
            }
        }

        return '\x1b';
    }

    return c;
}

int editor_process_cursor_movement(int key)
{
    erow *row = (EC.csrY >= EC.numRows) ? NULL : &EC.rows[EC.csrY];

    switch (key) {
        case LOWER_CASE_W:
        case ARROW_UP:
            if (EC.csrY > 0) {
                --EC.csrY;
            }
            break;
        case LOWER_CASE_A:
        case ARROW_LEFT:
            if (EC.csrX > 0) {
                --EC.csrX;
            } else if (EC.csrY > 0) {
                --EC.csrY;
                erow *newRow = (EC.csrY >= EC.numRows) ? NULL : &EC.rows[EC.csrY];
                if (newRow && newRow->size > 0) {
                    switch (currEditingMode) {
                        case VIEW:
                            EC.csrX = newRow->size - 1;
                            break;
                        case EDIT:
                            EC.csrX = newRow->size;
                            break;
                    }
                }
            }
            return 0;
        case LOWER_CASE_S:
        case ARROW_DOWN:
            if (EC.csrY < EC.numRows) {
                ++EC.csrY;
            }
            break;
        case LOWER_CASE_D:
        case ARROW_RIGHT:
            if (row) {
                if (EC.csrX < row->size - (currEditingMode == VIEW)) {
                    ++EC.csrX;
                } else if (EC.csrY < EC.numRows) {
                    ++EC.csrY;
                    EC.csrX = 0;
                }
            }
            return 0;
    }

    erow *newRow = (EC.csrY >= EC.numRows) ? NULL : &EC.rows[EC.csrY];
    if (!newRow || newRow->size == 0) {
        EC.csrX = 0;
    } else if (EC.csrX >= newRow->size) {
        switch (currEditingMode) {
            case VIEW:
                EC.csrX = newRow->size - 1;
                break;
            case EDIT:
                EC.csrX = newRow->size;
                break;
        }
    }

    return 0;
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
            if (EC.csrX > 0) {
                --EC.csrX;
            }
            break;

        // ENTER aka carriage return
        case '\r':
            // TODO
            break;

        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            // TODO
            editor_delete_char();
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
            editor_insert_char(c);
            break;
    }
}

int editor_process_keypress(void)
{
    int c = editor_read_keypress();

    switch (c) {
        case PAGE_UP:
        case PAGE_DOWN: {
            if (c == PAGE_UP) {
                EC.csrY = EC.rowOff;
            } else if (c == PAGE_DOWN) {
                EC.csrY = EC.rowOff + EC.screenRows - 1;
                if (EC.csrY >= EC.numRows) {
                    EC.csrY = EC.numRows - 1;
                }
            }

            int times = EC.screenRows;
            while (times--) {
                editor_process_cursor_movement(c == PAGE_UP ? ARROW_UP :
                                               ARROW_DOWN);
            }
            break;
        }

        case HOME_KEY:
            EC.csrX = 0;
            break;
        case END_KEY:
            if (EC.csrY < EC.numRows) {
                switch (currEditingMode) {
                    case VIEW:
                        EC.csrX = EC.rows[EC.csrY].size - 1;
                        break;
                    case EDIT:
                        EC.csrX = EC.rows[EC.csrY].size;
                        break;
                }
            }
            break;

    }
    
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
    editor_scroll();

    // Clears everything
    abuf ab = ABUF_INIT;

    ab_append(&ab, "\x1b[?25l", 6);
    ab_append(&ab, "\x1b[H", 3);

    editor_draw_rows(&ab);
    editor_draw_status_bar(&ab);

    editor_move_cursor(&ab, (EC.csrY - EC.rowOff) + 1, 
                      (EC.rdrX - EC.colOff) + 1);
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

void editor_draw_status_bar(abuf *ab)
{
    ab_append(ab, "\x1b[7m", 4);

    // Draw welcome message
    char welcome[80];
    if (EC.numRows == 0) {
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

        padding = (EC.screenCols - welcomelen) / 2;
        while (padding--) {
            ab_append(ab, " ", 1);
        }

    } else {
        char rstatus[80];
        int fnameLen = snprintf(welcome, sizeof(welcome), "%.20s - %d lines",
                           EC.filename ? EC.filename : "[No Name]", EC.numRows);
        int rlen = snprintf(rstatus, sizeof(welcome), "%d;%d / %d",
                            EC.csrY, EC.rdrX, EC.numRows);
        if (fnameLen > EC.screenCols) {
            fnameLen = EC.screenCols;
        }
        ab_append(ab, welcome, fnameLen);

        while (fnameLen < EC.screenCols) {
            if (EC.screenCols - fnameLen == rlen) {
                ab_append(ab, rstatus, rlen);
                break;
            } else {
                ab_append(ab, " ", 1);
                ++fnameLen;
            }
        }
    }
    ab_append(ab, "\x1b[m", 3);

}

int editor_csrx_to_rdrx(erow *row, int csrX) {
    int rx = 0;
    for (int i = 0; i < csrX; ++i) {
        if ((row->chars)[i] == '\t') {
            rx += (TAB_STOP - 1) - (rx % TAB_STOP);
        }
        ++rx;
    }
    return rx;
}

void editor_scroll()
{
    // Just in case csrY is not in a row, x must then be 0
    EC.rdrX = 0;
    if (EC.csrY < EC.numRows) {
        EC.rdrX = editor_csrx_to_rdrx(&EC.rows[EC.csrY], EC.csrX);
    }

    if (EC.csrY < EC.rowOff) {
        EC.rowOff = EC.csrY;
    }
    if (EC.csrY >= EC.rowOff + EC.screenRows - LAST_ROW_OFF) {
        EC.rowOff = EC.csrY - EC.screenRows + 1 + LAST_ROW_OFF;
    }

    if (EC.rdrX < EC.colOff) {
        EC.colOff = EC.rdrX;
    }
    if (EC.rdrX >= EC.colOff + EC.screenCols - LAST_COL_OFF) {
        EC.colOff = EC.rdrX - EC.screenCols + 1 + LAST_COL_OFF;
    }
}

int editor_draw_rows(abuf *ab)
{
    // Draw rows
    for (int y = 0; y < EC.screenRows - 1; ++y) {
        int filerow = y + EC.rowOff;
        if (filerow < EC.numRows) {
            int len = EC.rows[filerow].rsize - EC.colOff;
            if (len < 0) {
                len = 0;
            }
            if (len > EC.screenCols) {
                len = EC.screenCols;
            }
            ab_append(ab, &EC.rows[filerow].render[EC.colOff], len);
        } else {
            ab_append(ab, "~", 1);
        }
        ab_append(ab, "\x1b[K\r\n", 5);
    }

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
