#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "global.h"
#include "editor_io.h"
#include "terminal.h"

#define CHAD_VERSION "0.0.1"
#define CHAD_QUIT_TIMES 3
#define LAST_ROW_OFF 1
#define LAST_COL_OFF 0
#define TAB_STOP 8

// TODO: Make macros for printKeysMode OR combine the modes together
int printKeysMode = 0;
int currEditingMode = VIEW;

/*** FILE_IO ***/

char *editor_rows_to_string(int *buflen)
{
    int totlen = 0;
    for (int i = 0; i < EC.numRows; ++i) {
        totlen += EC.rows[i].size + 1;
    }

    *buflen = totlen;

    char *buf = malloc(sizeof(*buf) * (totlen));
    char *p = buf;

    for (int i = 0; i < EC.numRows; ++i) {
        memcpy(p, EC.rows[i].chars, EC.rows[i].size);
        p += EC.rows[i].size;
        *p = '\n';
        ++p;
    }

    return buf;
}

char *editor_prompt(const char *prompt)
{
    size_t bufsz = 128;
    char *buf = malloc(sizeof(*buf) * bufsz);

    size_t buflen = 0;
    buf[0] = '\0';

    while (1) {
        editor_set_status_message(prompt, buf);
        editor_refresh_screen();

        int c = editor_read_keypress();

        if (c == DEL_KEY || c == BACKSPACE) {
            if (buflen != 0) {
                buf[--buflen] = '\0';
            }
        } else if (c == '\x1b') {
            editor_set_status_message("");
            free(buf);
            return NULL;
        } else if (c == '\r') {
            if (buflen != 0) {
                editor_set_status_message("");
                return buf;
            }
        } else if (!iscntrl(c) && c < 128) {
            if (buflen == bufsz - 1) {
                bufsz *= 2;
                buf = realloc(buf, bufsz);
            }

            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
    }
}

void editor_save()
{
    if (EC.filename == NULL) {
        EC.filename = editor_prompt("Save as: %s");
        if (EC.filename == NULL) {
            editor_set_status_message("Save aborted");
            return;
        }
    }

    int len;
    char *buf = editor_rows_to_string(&len);

    errno = 0;
    int fd = open(EC.filename, O_RDWR | O_CREAT, 0644);
    if (fd == -1 && errno != 0) {
        free(buf);
        editor_set_status_message("Can't save! I/O error: %s", strerror(errno));
        die("open, error in function editor_save", errno);
    }

    errno = 0;
    if (ftruncate(fd, len) == -1 && errno != 0) {
        close(fd);
        free(buf);
        editor_set_status_message("Can't save! I/O error: %s", strerror(errno));
        die("ftruncate, error in function editor_save", errno);
    }

    if (write(fd, buf, len) != len) {
        close(fd);
        free(buf);
        editor_set_status_message("Can't save! I/O error: %s", strerror(errno));
        die("write, error in function in editor_save", errno);
    }

    editor_set_status_message("%d bytes written to disk", len);

    close(fd);
    free(buf);

    EC.dirty = 0;
}

// MUST ONLY BE CALLED IN EDIT MODE (currEditingMode)
void editor_row_insert_char(erow *row, int at, int c)
{
    if (at < 0) {
        at = 0;
    } else if (at > row->size) {
        at = row->size;
    }

    errno = 0;
    row->chars = realloc(row->chars, row->size + 2);
    if (row->chars == NULL && errno != 0) {
        die("realloc, error in function editor_row_insert_char", errno);
    }

    errno = 0;
    memmove(&(row->chars[at + 1]), &(row->chars[at]), row->size - at + 1);
    if (row->chars == NULL && errno != 0) {
        die("memmove, error in function editor_row_insert_char", errno);
    }
    
    ++row->size;
    row->chars[at] = c;
    editor_update_row(row);
    
    ++EC.dirty;
    
}

void editor_row_append_string(erow *row, char *s, size_t len)
{
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&(row->chars[row->size]), s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editor_update_row(row);
    ++EC.dirty;
}

void editor_insert_char(int c)
{
    if (EC.csrY == EC.numRows) {
        editor_insert_row(EC.numRows - 1, "", 0);
    }
    editor_row_insert_char(&EC.rows[EC.csrY], EC.csrX, c);
    ++EC.csrX;
}

void editor_row_delete_char(erow *row, int at)
{
    if (at < 0 || at >= row->size) {
        return;
    }
    errno = 0;
    memmove(&(row->chars[at]), &(row->chars[at + 1]), row->size - at);
    // TODO: Probably wrong way to ERROR check for memmove, instead look at
    // memmove docs and check for return val.
    if (row->chars == NULL && errno != 0) {
        die("memmove, error in function editor_row_delete_char", errno);
    }

    errno = 0;

    --row->size;
    editor_update_row(row);

    ++EC.dirty;
}

// For deletion functions, we will memmove first and then realloc, since we will
// lose the end data if we realloc first
void editor_delete_char()
{
    if (EC.csrY == EC.numRows) {
        return;
    }
    if (EC.csrX == 0 && EC.csrY == 0) {
        return;
    }

    erow *row = &(EC.rows[EC.csrY]);
    if (EC.csrX > 0) {
        editor_row_delete_char(row, EC.csrX - 1);
        --EC.csrX;
    } else {
        EC.csrX = EC.rows[EC.csrY - 1].size;
        editor_row_append_string(&(EC.rows[EC.csrY - 1]), row->chars, row->size);
        editor_delete_row(EC.csrY);
        --EC.csrY;
        return;
    }

}

void editor_free_row(erow *row)
{
    free(row->render);
    free(row->chars);
}

void editor_delete_row(int currRow)
{
    errno = 0;
    editor_free_row(&(EC.rows[currRow]));
    memmove(&(EC.rows[currRow]), &(EC.rows[currRow + 1]), 
           (sizeof(erow) * (EC.numRows - currRow - 1)));
    if (EC.rows == NULL && errno != 0) {
        die("memmove, error in function editor_delete_row", errno);
    }

    errno = 0;
    EC.rows = realloc(EC.rows, sizeof(erow) * (EC.numRows - 1));

    if (EC.rows == NULL && errno != 0) {
        die("realloc, error in function editor_delete_row", errno);
    }

    --EC.numRows;
    ++EC.dirty;
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
        die("malloc, error in function editor_update_row", errno);
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

void editor_insert_row(int at, const char *s, size_t len)
{
    if (at < 0 || at > EC.numRows) {
        return;
    }

    errno = 0;
    EC.rows = realloc(EC.rows, sizeof(erow) * (EC.numRows + 1));

    if (EC.rows == NULL && errno != 0) {
        die("realloc, error in function editor_insert_row", errno);
    }

    errno = 0;
    memmove(&(EC.rows[at + 1]), &(EC.rows[at]), 
        sizeof(erow) * (EC.numRows - at));
    if (EC.rows == NULL && errno != 0) {
        die("memmove, error in function editor_insert_row", errno);
    }

    EC.rows[at].size = len;
    if ((EC.rows[at].chars = malloc(len + 1)) == NULL) {
        die("malloc, error in function editor_insert_row", errno);
    }

    erow *newRow = &EC.rows[at];
    memcpy(newRow->chars, s, len);
    (newRow->chars)[len] = '\0';

    newRow->rsize = 0;
    newRow->render = NULL;
    editor_update_row(newRow);

    ++EC.numRows;
    ++EC.dirty;
}

void editor_insert_new_line(void)
{
    if (EC.csrX == 0) {
        editor_insert_row(EC.csrY, "", 0);
    } else {
        erow *row = &EC.rows[EC.csrY];
        editor_insert_row(EC.csrY + 1, &(row->chars)[EC.csrX], 
                                       row->size - EC.csrX);
        row = &EC.rows[EC.csrY];
        row->size = EC.csrX;
        row->chars[row->size] = '\0';
        editor_update_row(row);
    }

    ++EC.csrY;
    EC.csrX = 0;
}

void editor_open(const char *filename)
{
    free(EC.filename);
    EC.filename = strdup(filename);
    FILE *fp;
    if (file_perm_exists(filename, RD_AC)) {
        EC.fileExists = 1;
        file_perm_exists(filename, RDWR_AC);
    } else {
        EC.fileExists = 0;
        return;
    }
    fp = fopen(filename, "r");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&line, &linecap, fp)) != EOF) {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                               line[linelen - 1] == '\r')) {
            --linelen;
        }
        editor_insert_row(EC.numRows, line, linelen);
    }

    free(line);
    fclose(fp);

    EC.dirty = 0;
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
            die("read, error in function editor_read_char", errno);
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
            if (EC.csrY < EC.numRows - 1) {
                ++EC.csrY;
            }
            break;
        case LOWER_CASE_D:
        case ARROW_RIGHT:
            if (row) {
                if (EC.csrX < row->size - (currEditingMode == VIEW)) {
                    ++EC.csrX;
                } else if (EC.csrY < EC.numRows - 1) {
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
            editor_insert_new_line();
            break;


        /*** MOVING ***/
        case ARROW_UP:
        case ARROW_LEFT:
        case ARROW_DOWN:
        case ARROW_RIGHT:
            editor_process_cursor_movement(c);
            break;

        case CTRL_KEY('l'):
        case '\x1b':
            break;

        default:
            if (!iscntrl(c)) {
                editor_insert_char(c);   
            }
            break;
    }
}

int editor_process_keypress(void)
{
    static int quit_times = CHAD_QUIT_TIMES;

    int c = editor_read_keypress();

    switch (c) {
        case CTRL_KEY('q'):
            if (EC.dirty && quit_times > 0) {
                editor_set_status_message("WARNING!!! File might have unsaved"
                     " edits. Press <Ctrl><Q>"
                     " %d more times to quit.", quit_times);
                --quit_times;
                return 0;
            }
            editor_reset_screen();
            exit(0);
            break;
        case CTRL_KEY('s'):
            editor_save();
            return 0;

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
            return 0;
        }

        case HOME_KEY:
            EC.csrX = 0;
            return 0;
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
            return 0;

    }

     if (EC.fileExists && (EC.filePerms & WR_AC) != 0x02) {
         return 0;
     }

    switch (c) {
        case BACKSPACE:
        // Do we want to use <CTRL>+<H> ???
        // case CTRL_KEY('h'):
        case DEL_KEY:
            if (EC.csrX > EC.rows[EC.csrY].size) {
                return 0;
            }
            if (currEditingMode == VIEW) {
                ++EC.csrX;
            }
            editor_delete_char();
            if (currEditingMode == VIEW) {
                --EC.csrX;
            }
            return 0;
    }

    quit_times = CHAD_QUIT_TIMES;
    
    switch (currEditingMode) {
        case VIEW:
            editor_view_mode(c);
            break;
        case EDIT:
            editor_edit_mode(c);
            break;
        default:
            die("non-existent editingMode, error in function"
                 "editor_process_keypress", errno);
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
        die("write, error in function print_cursor_function", errno);
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
        die("write, error in editor_reset_screen", errno);
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
        die("write, error in editor_refresh_screen", errno);
    }

    ab_free(&ab);


    return 0;
}

void editor_set_status_message(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(EC.statusMessage, sizeof(EC.statusMessage), fmt, ap);
    va_end(ap);
    EC.statusMessageTime = time(NULL);
}

void editor_draw_status_bar(abuf *ab)
{
    ab_append(ab, "\x1b[7m", 4);

    if (strlen(EC.statusMessage) != 0) {
        int msglen = strlen(EC.statusMessage);
        if (msglen > EC.screenCols - LAST_COL_OFF) {
            msglen = EC.screenCols - LAST_COL_OFF;
        }
        if (msglen && time(NULL) - EC.statusMessageTime < 5) {
            ab_append(ab, EC.statusMessage, msglen);
            ab_append(ab, "\x1b[K", 3);
            while (EC.screenCols - msglen > 0) {
                ab_append(ab, " ", 1);
                ++msglen;
            }
            ab_append(ab, "\x1b[m", 3);
            return;
        } else {
            EC.statusMessage[0] = '\0';
            EC.statusMessageTime = 0;
        }
    }

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
        int fnameLen = snprintf(welcome, sizeof(welcome), "%.20s - %d lines%s",
                           EC.filename ? EC.filename : "[No Name]", EC.numRows,
                           EC.dirty ? " (modified)" : "");
        int rlen = snprintf(rstatus, sizeof(welcome), "%d;%d / %d",
                            EC.csrY, EC.rdrX, EC.numRows);
        if (fnameLen > EC.screenCols) {
            fnameLen = EC.screenCols;
        }
        ab_append(ab, welcome, fnameLen);
        ab_append(ab, "\x1b[K", 3);

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
