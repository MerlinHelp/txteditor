#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "global.h"
#include "terminal.h"
#include "txteditor.h"
#include "editor_io.h"

typedef struct abuf abuf;

void start()
{
    enable_raw_mode();
    get_terminal_dimensions(&EC.screenrows, &EC.screencols);

    editor_refresh_screen();


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
