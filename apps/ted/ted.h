#pragma once

typedef struct {
    int size;
    int cap;
    char data[];
} line_t;

typedef struct {
    line_t **line;
    int size;
    int cap;
} file_t;

typedef struct {
    struct {
        int offset; // Textarea offset on screen (x-direction)
        int size;   // Textarea size on screen (x-direction)
        int pos;    // Textarea relative cursor position (x-direction)
    } x;
    struct {
        int offset; // Textarea offset on screen (y-direction)
        int size;   // Textarea size on screen (y-direction)
        int pos;    // Textarea relative cursor position (y-direction)
    } y;
    struct {
        int offset; // Offset for horizontal scrolling
        int pos;    // Position in current line
    } wx;
    struct {
        int offset; // Offset for vertical scrolling
        int pos;    // Current line in file
    } wy;
    file_t *file;   // Data for textareaa
} textarea_t;

typedef struct {
    int rows;
    int cols;
    const char *name;
    textarea_t w;
} tui_t;

void tui_refresh(tui_t *tui);

line_t *line_grow(file_t *file, int num, int minsz);
void file_grow(file_t *file);
int file_write(const char *filename, file_t *file);
file_t *file_read(const char *filename);

void textarea_up(textarea_t *w, int num);
void textarea_down(textarea_t *w, int num);
void textarea_right(textarea_t *w);
void textarea_left(textarea_t *w);
void textarea_home(textarea_t *w);
void textarea_end(textarea_t *w);
void textarea_enter(textarea_t *w);
void textarea_backspace(textarea_t *w);
void textarea_glyph(textarea_t *w, int key);
