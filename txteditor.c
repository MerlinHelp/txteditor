#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "global.h"
#include "rawmode.h"
#include "txteditor.h"
#include "editor_io.h"

void start()
{
    enable_raw_mode();
    editor_refresh_screen();
    editor_move_cursor_to_top();

    while (1) {
        editor_process_keypress();
    }
    
    return;
}

int main(void)
{
    start();

    return EXIT_SUCCESS;
}
