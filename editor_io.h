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
int editor_read_keypress(void);
int editor_process_cursor_movement(int);
int editor_process_keypress(void);

/*** OUTPUT ***/
void ab_append(struct abuf*, const char*, int);
void ab_free(struct abuf*);
int editor_empty_screen(void);
int editor_reset_screen(void);
int editor_refresh_screen(void);
int editor_draw_empty_rows(struct abuf*);
int editor_move_cursor(struct abuf*, int, int);
int print_cursor_position(void);

#endif


#endif
