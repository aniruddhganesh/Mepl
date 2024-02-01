#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>

#include "client_mpd.h"
#include "input.h"
#include "command.h"
#include "main.h"

#define TEXT_WIN_PADDING 1

#define CTRL_KEY(c) ((c) & 037)

WINDOW *Window_ui = NULL;
WINDOW *Window_err = NULL;
WINDOW *Window_audio = NULL;

void exit_clean(int status, struct mpd_connection *conn)
{
    endwin();
    if (conn != NULL) {
        const char *msg = mpd_connection_get_error_message(conn);
        fprintf(stderr, "MPD error: %s\n", msg);
        mpd_connection_free(conn);
        exit(status);
    }

    exit(status);
}

static void initialize_colors(void)
{
    start_color();
    /*         Name     fg              bg   */
    init_pair(COL_CMD,      COLOR_BLACK,    COLOR_GREEN);
    init_pair(COL_AUD,      COLOR_BLACK,    COLOR_BLUE);
    init_pair(COL_AUD_INV,  COLOR_BLUE,     COLOR_BLACK);
    init_pair(COL_INFO,     COLOR_BLUE,     COLOR_BLACK);
    init_pair(COL_ERR,      COLOR_RED,      COLOR_BLACK);
}

void wprint_blank_line(WINDOW *win, const unsigned width, enum Colors COLOR)
{

    char str_fmt[8];
    snprintf(str_fmt, width, "%%%ds", width);
    wattron(win, COLOR_PAIR(COLOR));
    mvwprintw(win, 0, 0, str_fmt, " ");
    attroff(COLOR_PAIR(COLOR));
    wmove(win, 0, 0);
}

static void adjust_window_size(struct display *d)
{
    /* ----------------  2
     * |---audio------|
     * |              | 
     * |   win_ui     |  h - 3
     * |              |
     * |---input------| 1
     * =====err======== 1   */ 
    wclear(d->win_audio);
    wclear(d->win_input);
    wclear(d->win_ui);
    wclear(d->win_err);

    getmaxyx(stdscr, d->h, d->w);

    mvwin(d->win_audio, 0, 0);
    mvwin(d->win_ui, 2, 0);
    mvwin(d->win_input, d->h - 2, 0);
    mvwin(d->win_err, d->h - 1, 0);

    wresize(d->win_audio, 2 , d->w);
    wresize(d->win_ui, d->h - 4, d->w);
    wresize(d->win_input, 1, d->w);
    wresize(d->win_err, 1, d->w);

    box(d->win_ui, 0, 0);

    wprint_blank_line(d->win_input, d->w, COL_CMD);

    wrefresh(d->win_audio);
    wrefresh(d->win_ui);
    wrefresh(d->win_err);
}


static struct display init_terminal_ui(struct display *d)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, true);
    initialize_colors();

    d->win_ui = newwin(0,0,0,0);
    d->win_input = newwin(0,0,0,0);
    d->win_err = newwin(0,0,0,0);
    d->win_audio = newwin(0,0,0,0);

    Window_ui = d->win_ui;
    Window_err = d->win_err;
    Window_audio = d->win_audio;
    timeout(0);

    return *d;
}

static bool has_resized(struct display *d)
{
    int h, w;
    getmaxyx(stdscr, h, w);
    return (d->w != w || d->h != h);
}


void ui_print_error(enum Colors level, const char *fmt, ...)
{
    va_list ap;
    char *msg = malloc(strlen(fmt)*2);
    va_start(ap, fmt);
    vsprintf(msg, fmt, ap);
    va_end(ap);

    wclear(Window_err);
    wattron(Window_err, COLOR_PAIR(level));
    mvwprintw(Window_err, 0, 0, "%s", msg) ;
    wattroff(Window_err, COLOR_PAIR(level));
    wrefresh(Window_err);
}

static void display_ui_elements(struct display *d)
{
    wclear(Window_audio);
    const char *cur_song_name = get_current_playing();
    const char *volume_str = get_volume_str();

    int old_x = getcurx(d->win_input);

    if (cur_song_name != NULL) 
        mvwprintw(Window_audio, 0, (d->w - strlen(cur_song_name))/2, "%s", cur_song_name); 
    if (volume_str != NULL) {
        mvwprintw(Window_audio, 0, 0, "%s", volume_str);
    }
    wrefresh(Window_audio);
    wmove(d->win_input, old_x, 0);
}

int main() {
    static struct display display;
    display = init_terminal_ui(&display);
    init_mpd_connection();

    int i = 0;
    while (1) {
        if (has_resized(&display)) {
            // For some reason we need 2 calls
            adjust_window_size(&display);
            adjust_window_size(&display);
        }


        int x = freadline(&display, 50);

        if (x > 0) {
            wclear(Window_err); wrefresh(Window_err);

            process_input_command(display.input);
            display.input_len = 0;
            memset(display.input, 0, x);
        }


        display_ui_elements(&display);
        usleep(10000);
    }

    exit_clean(0, NULL);
}


