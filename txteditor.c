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
    get_terminal_dimensions(&EC.screenRows, &EC.screenCols);
    
    EC.csrX = 0;
    EC.csrY = 0;

    editor_refresh_screen();


    while (1) {
        editor_refresh_screen();
        (void)editor_process_keypress();
    }
    
    return;
}

int main(void)
{
    start();

    return EXIT_SUCCESS;
}
