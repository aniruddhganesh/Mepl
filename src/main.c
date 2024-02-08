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

#define UREFRESH 80000
#define UI_PADDING 2 

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
    init_pair(COL_AUD,      COLOR_BLUE,     COLOR_BLACK);
    init_pair(COL_UI,       COLOR_GREEN,    COLOR_BLACK);
    init_pair(COL_PROG,     COLOR_MAGENTA,  COLOR_BLACK);
    init_pair(COL_INFO,     COLOR_BLUE,     COLOR_BLACK);
    init_pair(COL_ERR,      COLOR_RED,      COLOR_BLACK);
}


void wprint_line(WINDOW *win,const unsigned y, const unsigned width, enum Colors COLOR, const char* c)
{
    char str_fmt[8];
    snprintf(str_fmt, width, "%%%ds", width);
    wattron(win, COLOR_PAIR(COLOR));
    mvwprintw(win, y, 0, str_fmt, c);
    attroff(COLOR_PAIR(COLOR));
}
void wprint_blank_line(WINDOW *win, const unsigned width, enum Colors COLOR)
{
    wprint_line(win, 0, width, COLOR, " ");
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
    free(msg);

    wattroff(Window_err, COLOR_PAIR(level));
    wrefresh(Window_err);
}

void ui_clear_scrn(void)
{
    wclear(Window_ui);
    box(Window_ui, 0, 0);
    wrefresh(Window_ui);
    return;
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

    wattron(Window_ui, COLOR_PAIR(COL_UI));
    mvwprintw(Window_ui, i++, UI_PADDING, "%s", msg);
    wrefresh(Window_ui);
    wattroff(Window_ui, COLOR_PAIR(COL_UI));

    free(msg);
}

static void display_ui_elements(struct display *d)
{
    wclear(Window_audio);
    char *cur_song_name = getstr_current_playing(MPD_TAG_TITLE);
    char *volume_str = getstr_volume();

    int old_x = getcurx(d->win_input);

    wattron(Window_audio, COLOR_PAIR(COL_AUD));

    if (cur_song_name) {
        int align_middle = (d->w - strlen(cur_song_name)) / 2;
        mvwprintw(Window_audio, 0, align_middle, "%s", cur_song_name); 
        free(cur_song_name);
    }

    if (volume_str)  {
        mvwprintw(Window_audio, 0, 0, "%s", volume_str);
        free(volume_str);
    }

    unsigned elaps;
    unsigned dur;
    if (get_song_position_on_duration(&elaps, &dur)) {
        // "[mm:ss/mm:ss]" -> 13
        mvwprintw(Window_audio, 0, d->w - 13, "[%.2d:%.2d/%.2d:%.2d]", 
                elaps / 60, elaps % 60, dur / 60, dur % 60); 
        // Progress bar
        wattron(Window_audio, COLOR_PAIR(COL_PROG));
        unsigned perc_elaps = ((float)elaps/(float)dur) * d->w;

        for (int i = 0; i < d->w; i++) {
            if (i < perc_elaps) {
                mvwprintw(Window_audio, 1, i, "=");
            } else if (i == perc_elaps) {
                mvwprintw(Window_audio, 1, i, ">");
            } else {
                mvwprintw(Window_audio, 1, i, "-");
            }
        }
        wattroff(Window_audio, COLOR_PAIR(COL_PROG));
    }

    wrefresh(d->win_input);
    wrefresh(Window_audio);
}

int main() {
    static struct display display;
    display = init_terminal_ui(&display);
    init_mpd_connection();

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
        usleep(UREFRESH);
    }

    exit_clean(0, NULL);
}


