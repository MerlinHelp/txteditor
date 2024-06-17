#ifndef EDITOR_IO_H
#define EDITOR_IO_H

#ifndef EDITOR_IO_FUNCS
#define EDITOR_IO_FUNCS

typedef struct abuf abuf;

/*** FILE_IO ***/
void editor_append_rows(const char*, size_t);
void editor_open(const char*);

/*** INPUT ***/
int editor_read_keypress(void);
int editor_process_cursor_movement(int);
int editor_process_keypress(void);

/*** OUTPUT ***/
void ab_append(abuf*, const char*, int);
void ab_free(abuf*);
int editor_empty_screen(void);
int editor_reset_screen(void);
int editor_refresh_screen(void);
void editor_scroll(void);
int editor_draw_rows(abuf*);
int editor_move_cursor(abuf*, int, int);
int print_cursor_position(void);

#endif


#endif
