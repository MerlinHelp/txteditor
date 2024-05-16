#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "global.h"
#include "terminal.h"
#include "txteditor.h"
#include "editor_io.h"

void start()
{
    enable_raw_mode();
    editor_refresh_screen();
    editor_move_cursor_to_top();

    if (get_terminal_dimensions(&EC.screenrows, &EC.screencols) == -1) {
        die("error in function get_terminal_dimensions");
    }

    editor_draw_empty_rows();
    editor_move_cursor_to_top();

    while (1) {
        (void)editor_process_keypress();
    }
    
    return;
}

int main(void)
{
    start();

    return EXIT_SUCCESS;
}
