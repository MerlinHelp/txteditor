#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "global.h"
#include "terminal.h"

// Keep original termios to revert back to after disable_raw_mode()
struct termios orignalTermios;

void disable_raw_mode(void)
{
    // Termios func (tcsetattr) to set terminal attributes
    if ((tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios)) == -1) {
        die("tcsetattr, error in function disable_raw_mode");
    }
}

void enable_raw_mode(void)
{
    struct termios raw;

    // Termios func (tcgetattr) to get terminal attributes
    // We check func return for errors
    if ((tcgetattr(STDIN_FILENO, &raw)) == -1) {
        die("tcgetattr, error in function enable_raw_mode");
    }
    if ((tcgetattr(STDIN_FILENO, &originalTermios)) == -1) {
        die("tcgetattr, error in function enable_raw_mode");
    }

    // stdlib.h func to run disable_raw_mode at program exit
    atexit(disable_raw_mode);

    /* Change different flag/attributes of terminal
     * By using Bitwise operator '&' with '=':
     *     We can enable/disable flags
     *
     * c_lflag: local flags, c_iflag: input flag, c_oflag: output flag,
     * c_cflag: control flag, c_cc: control character byte array
     */

    /* Different flags/attributes you can change
     * c_lflag:
     *     ECHO: switch whether stdin gets echoed
     *     ICANON: switch canonical mode
     *     IEXTEN: switch 'Ctrl-V' special behavior
     *     ISIG: switch Terminate ('Ctrl-C') / Suspend ('Ctrl-Z')
     * c_iflag:
     *     IXON: switch software flow control ('Ctrl-S' and 'Ctrl-Q')
     *     ICRNL: switch input translation of '\r' -> '\n' ('Ctrl-M')  
     *     BREAKINT: switch accepting (ON) / ignoring (OFF) BREAK condition on input
     *     INPCK: switch input parity checking
     *     ISTRIP: switch bit-stripping [(8 bits -> 7 bits) means ON else 8 bits]
     * c_oflag:
     *     OPOST: switch output translation of '\r' -> "\r\n" (ON)
     * c_cflag:
     *     CSIZE: bitmask representing the char size in c_cflag
     *     CS8: bitmask constant representing char size of 8
     * c_cc:
     *     VMIN: Minimum bytes of input needed before read() returns
     *     VTIME: max time waiting for read() before timeout (tenths of a second)
     */
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag &= ~(CSIZE);
    raw.c_cflag |= (CS8);

    raw.c_cc[VMIN] = 0; // Minimum of 0 bytes, read instantly once there is input
    raw.c_cc[VTIME] = 1; // Maximum time of 0.1s before read times out


    if ((tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw)) == -1) {
        die("tcsetattr, error in function enable_raw_mode");
    }
}

// TODO, need to figure out how to get the two dimensions back to the main file.
int get_terminal_dimensions(char* buf, const char* cmd)
{
    FILE *in;
    extern FILE *popen();

    if (!(in = popen(cmd, "r"))) {
        die("popen, error in function get_terminal_decisions");
    }
    
    pclose(in);

    return 0;

}
