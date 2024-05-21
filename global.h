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

#ifndef TERMIOS_H
#define TERMIOS_H

#include <termios.h>

#endif
struct editorConfig {
    int screenrows;
    int screencols;
    struct termios originalTermios;
};

extern struct editorConfig EC;

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
#define ENTER 0x0d
#define LOWER_CASE_W 0x77
#define LOWER_CASE_A 0x61
#define LOWER_CASE_S 0x73
#define LOWER_CASE_D 0x44

#endif
