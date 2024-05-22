#ifndef RAWMODE_H
#define RAWMODE_H

#ifndef RAWMODE_FUNCS
#define RAWMODE_FUCNS

void disable_raw_mode(void);
void enable_raw_mode(void);
int get_terminal_dimensions(int*, int*);
int get_cursor_position(int*, int*, int*);

#endif

#endif
