#ifndef MAIN_H
#define MAIN_H
#include <ncurses.h>
#include <mpd/client.h>

struct display {
    size_t w, h;
    char *input;

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
    COL_AUD_INV,
    COL_INFO,
    COL_ERR,
};

void exit_clean(int status, struct mpd_connection *conn);
void ui_print_error(enum Colors level, const char *fmt, ...);

#endif
