#ifndef EDITOR_IO_H
#define EDITOR_IO_H

#ifndef EDITOR_IO_FUNCS
#define EDITOR_IO_FUNCS

/*** INPUT ***/
char editor_read_keypress(void);
int editor_process_keypress(void);

/*** OUTPUT ***/
int editor_refresh_screen(void);
int editor_draw_empty_rows(void);
int editor_move_cursor(int, int);
int editor_move_cursor_to_top(void);
int editor_move_cursor_next_line(void);
int editor_move_cursor_up(void);
int editor_move_cursor_left(void);
int editor_move_cursor_down(void);
int editor_move_cursor_right(void);

#endif

#endif
