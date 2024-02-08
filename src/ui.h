#ifndef UI_H
#define UI_H
#include <ncurses.h>

enum Colors {
    COL_CMD = 1,
    COL_AUD,
    COL_UI,
    COL_UI_ALT,
    COL_PROG,
    COL_INFO,
    COL_ERR,
};

void refresh_ui_state(void);
void ui_wprint_line(WINDOW *win,const unsigned y, 
        const unsigned width, enum Colors COLOR, const char* c);
void ui_wprint_blank_line(WINDOW *win, const unsigned width, enum Colors COLOR);

void ui_print_error(enum Colors level, const char *fmt, ...);
void ui_print_str(bool clear, const char *fmt, ...);
void ui_clear_scrn(void);
#endif
