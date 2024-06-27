#ifndef FEATURE_TEST_MACROS
#define FEATURE_TEST_MACROS

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "global.h"
#include "terminal.h"
#include "txteditor.h"
#include "editor_io.h"

void init_editor_config(void)
{
    EC.csrX = 0;
    EC.csrY = 0;
    EC.rdrX = 0;
    EC.rowOff = 0;
    EC.colOff = 0;
    EC.numRows = 0;
    EC.rows = NULL;
    EC.dirty = 0;
    EC.filename = NULL;
    EC.statusMessage[0] = '\0';
    EC.statusMessageTime = 0;
    
    get_terminal_dimensions(&EC.screenRows, &EC.screenCols);
}

int main(int argc, char **argv)
{
    enable_raw_mode();
    init_editor_config();

    if (argc >= 2) {
        editor_open(*(argv + 1));
    }

    editor_set_status_message("HELP: <Ctrl><S> = save | <Ctrl><Q> = quit");

    while (1) {
        get_terminal_dimensions(&EC.screenRows, &EC.screenCols);
        editor_refresh_screen();
        (void)editor_process_keypress();
    }

    return EXIT_SUCCESS;
}
