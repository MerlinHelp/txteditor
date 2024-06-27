#ifndef GLOBAL_H
#define GLOBAL_H

#ifndef GLOBAL_FUNCS
#define GLOBAL_FUNCS

void die(const char*);
int num_places(int);

#endif

#ifndef GLOBAL_VARS
#define GLOBAL_VARS
extern const char *getNumTerminalRows;
extern int currEditingMode;
extern int printKeysMode;

#ifndef TERMIOS_H
#define TERMIOS_H

#include <termios.h>

#endif

#ifndef GLOBAL_STRUCT
#define GLOBAL_STRUCT

typedef struct abuf {
    char *b;
    int len;
} abuf;

#define ABUF_INIT {NULL, 0}

typedef struct erow {
    int size;
    int rsize;
    char *chars;
    char *render;
} erow;

typedef struct editorConfig {
    int csrX, csrY;
    int rdrX;
    int rowOff;
    int colOff;
    int screenRows;
    int screenCols;
    int numRows;
    erow *rows;
    int dirty;
    char *filename;
    char statusMessage[80];
    int statusMessageTime;
    struct termios originalTermios;
} editorConfig;

extern editorConfig EC;

#endif

#endif

#ifndef GLOBAL_MACROS
#define GLOBAL_MACROS

/*** Editor modes ***/
#define VIEW 0
#define EDIT 1

#endif

/*** Keys ***/

/** Cmd keys **/
#define CTRL_KEY(k) ((k) & (0x1f))

/** Mv keys **/
#define LOWER_CASE_W 0x77
#define LOWER_CASE_A 0x61
#define LOWER_CASE_S 0x73
#define LOWER_CASE_D 0x64
enum editorKey {
    BACKSPACE = 127,
    ENTER,
    ARROW_UP = 1000,
    ARROW_LEFT,
    ARROW_DOWN,
    ARROW_RIGHT,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

#endif
