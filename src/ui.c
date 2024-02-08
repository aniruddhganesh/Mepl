#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"
#include "main.h"

void ui_wprint_line(WINDOW *win, const unsigned y, 
        const unsigned width, enum Colors COLOR, const char* c)
{
    char str_fmt[8];
    snprintf(str_fmt, width, "%%%ds", width);
    wattron(win, COLOR_PAIR(COLOR));
    mvwprintw(win, y, 0, str_fmt, c);
    attroff(COLOR_PAIR(COLOR));
}

void ui_wprint_blank_line(WINDOW *win, const unsigned width, enum Colors COLOR)
{
    ui_wprint_line(win, 0, width, COLOR, " ");
}

void ui_print_error(enum Colors level, const char *fmt, ...)
{
    va_list ap;
    char *msg = malloc(strlen(fmt)*2);
    va_start(ap, fmt);
    vsprintf(msg, fmt, ap);
    va_end(ap);

    wclear(display.win_err);
    wattron(display.win_err, COLOR_PAIR(level));

    mvwprintw(display.win_err, 0, 0, "%s", msg) ;
    free(msg);

    wattroff(display.win_err, COLOR_PAIR(level));
    wrefresh(display.win_err);
}

void ui_print_str(bool clear, const char *fmt, ...)
{
    /* C makes it 0 only first time */
    static int i = 1;


    va_list ap;
    char *msg = malloc(strlen(fmt)*2);
    va_start(ap, fmt);
    vsprintf(msg, fmt, ap);
    va_end(ap);

    if (clear) {
        i = 0;
        ui_clear_scrn();
    }

    mvwprintw(display.win_ui, i++, 2, "%s", msg);
    wrefresh(display.win_ui);

    free(msg);
}

void ui_clear_scrn(void)
{
    wclear(display.win_ui);
    box(display.win_ui, 0, 0);
    wrefresh(display.win_ui);
    return;
}



