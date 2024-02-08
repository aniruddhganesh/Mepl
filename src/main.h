#ifndef MAIN_H
#define MAIN_H
#include <ncurses.h>
#include <mpd/client.h>

#include "command.h"

struct display {
    size_t w, h;
    char *input;
    size_t input_len;
    enum Print_State ui_state;

    WINDOW *win_ui;        /* Public access with Window_ui    */
    WINDOW *win_audio;     /* Public access with Window_audio */ 
    WINDOW *win_input;
    WINDOW *win_err;       /* Public access with Window_err   */
};

extern struct display display;



void exit_clean(int status, struct mpd_connection *conn);

#endif
