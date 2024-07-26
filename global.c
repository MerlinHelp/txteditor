#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "editor_io.h"
#include "global.h"

const char *getNumTerminalRows = "tput lines";

/* Helper Function: returns whether file specified by fname exists
 *      file_exists(const char*)
 *      returns 1 if fname exists
 *              0 else
 *
 */
int file_perm_exists(const char *fname, int amode)
{
    FILE *file;

    switch (amode) {
        case RD_AC:
            if ((file = fopen(fname, "r"))) {
                EC.filePerms = RD_AC;
                fclose(file);
                return 1;
            }
            break;
        case WR_AC:
            if ((file = fopen(fname, "w"))) {
                EC.filePerms = WR_AC;
                fclose(file);
                return 1;
            }
            break;
        case RDWR_AC:
            if ((file = fopen(fname, "r"))) {
                fclose(file);
                if ((file = fopen(fname, "a"))) {
                    EC.filePerms = RDWR_AC;
                    fclose(file);
                    return 1;
                }
            }
            break;
    }

    return 0;
}


void die(const char* s, int err)
{
    editor_refresh_screen();

    size_t nbytes = snprintf(NULL, 0, "%s. Errno error: %s", s, strerror(err)) + 1;
    char buf[nbytes];
    snprintf(buf, nbytes, "%s. Errno error: %s", s, strerror(err));

    perror(buf);
    exit(1);
}

/* Helper Function: returns numdigits of an int 
 *     num_places(int)
 * 
 */
 int num_places (int n) {
    if (n < 0) n = (n == INT_MIN) ? INT_MAX : -n;
    if (n < 10) return 1;
    if (n < 100) return 2;
    if (n < 1000) return 3;
    if (n < 10000) return 4;
    if (n < 100000) return 5;
    if (n < 1000000) return 6;
    if (n < 10000000) return 7;
    if (n < 100000000) return 8;
    if (n < 1000000000) return 9;
    /*      2147483647 is 2^31-1 - add more ifs as needed
       and adjust this final return as well. */
    return 10;
}
