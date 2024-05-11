#ifndef GLOBAL_H
#define GLOBAL_H

#ifndef GLOBAL_FUNCS
#define GLOBAL_FUNCS

void die(const char*);

#endif

#ifndef GLOBAL_VARS
#define GLOBAL_VARS
const char *getNumTerminalRows;

/*** Keys ***/
#define CTRL_KEY(k) ((k) & (0x1f))
#define ENTER 0x0d

#endif
