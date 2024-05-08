#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "global.h"
#include "rawmode.h"
#include "txteditor.h"
#include "editor_io.h"

int main(void)
{
    enable_raw_mode();

    while (1) {
        editor_process_keypress();
    }

    return EXIT_SUCCESS;
}
