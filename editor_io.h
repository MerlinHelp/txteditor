#ifndef EDITOR_IO_H
#define EDITOR_IO_H

#ifndef EDITOR_IO_FUNCS
#define EDITOR_IO_FUNCS

typedef struct abuf abuf;
typedef struct erow erow;

/*** FILE_IO ***/
char *editor_rows_to_string(int*);
void editor_save(void);
void editor_row_insert_char(erow*, int, int);
void editor_insert_char(int);
void editor_update_row(erow*);
void editor_insert_row(int, const char*, size_t);
void editor_insert_new_line(void);
void editor_row_delete_char(erow*, int);
void editor_delete_char(void);
void editor_delete_row(int);
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
void editor_set_status_message(const char*, ...);
void editor_draw_status_bar(abuf*);
void editor_scroll(void);
int editor_draw_rows(abuf*);
int editor_move_cursor(abuf*, int, int);
int print_cursor_position(void);

#endif


#endif
