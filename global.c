#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "editor_io.h"

const char *getNumTerminalRows = "tput lines";

void die(const char* s)
{
    editor_refresh_screen();
    editor_move_cursor_to_top();

    perror(s);
    exit(1);
}
