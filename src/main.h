#ifndef MAIN_H
#define MAIN_H
#include <ncurses.h>
#include <mpd/client.h>

struct display {
    size_t w, h;
    char *input;
    size_t input_len;

    WINDOW *win_ui;        /* Public access with Window_ui    */
    WINDOW *win_audio;     /* Public access with Window_audio */ 
    WINDOW *win_input;
    WINDOW *win_err;       /* Public access with Window_err   */
};


extern WINDOW *Window_ui;
extern WINDOW *Window_err;

enum Colors {
    COL_CMD = 1,
    COL_AUD,
    COL_UI,
    COL_UI_ALT,
    COL_PROG,
    COL_INFO,
    COL_ERR,
};


void exit_clean(int status, struct mpd_connection *conn);
void ui_clear_scrn(void);
void ui_print_error(enum Colors level, const char *fmt, ...);
void ui_print_str(bool clear, const char *fmt, ...);
void wprint_blank_line(WINDOW *win, const unsigned width, enum Colors COLOR);

#endif
