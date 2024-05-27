#ifndef EDITOR_IO_H
#define EDITOR_IO_H

#ifndef STRUCT
#define STRUCT

struct abuf {
    char *b;
    int len;
};

#define ABUF_INIT {NULL, 0}

#endif

#ifndef EDITOR_IO_FUNCS
#define EDITOR_IO_FUNCS

/*** INPUT ***/
char editor_read_keypress(void);
int editor_process_keypress(void);

/*** OUTPUT ***/
int editor_empty_screen(void);
int editor_reset_screen(void);
int editor_refresh_screen(void);
int editor_draw_empty_rows(struct abuf*);
int editor_move_cursor(int, int);
int editor_move_cursor_to_top(void);
int editor_move_cursor_next_line(void);
int editor_move_cursor_up(void);
int editor_move_cursor_left(void);
int editor_move_cursor_down(void);
int editor_move_cursor_right(void);
int print_cursor_position(void);

#endif


#endif
