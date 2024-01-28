#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>

#include "client_mpd.h"
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

static void wprint_blank_line(WINDOW *win, const unsigned width, enum Colors COLOR)
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

static int freadline(char **str, const int block_len, struct display *d)
{
    int i = 0;
    int ch = '\0';

    for (int j = 0;; j += block_len) {
        *str = realloc(*str, j + block_len);
        while ((ch = wgetch(d->win_input))) {
            switch (ch) {
                case '\n':                                  /*   Newline    */
                    wclear(d->win_input);
                    wprint_blank_line(d->win_input, d->w, COL_CMD);
                    (*str)[i++] = '\0';
                    return i;
                case '\t': 
                    // TODO: Implement tab autocomplete
                    break;
                case CTRL_KEY('w'):                         /* Delete word  */
                    while (i > 0) {
                        (*str)[i--] = '\0';
                        mvwprintw(d->win_input, 0, i, " ");
                        wmove(d->win_input, 0, i);
                        if ((*str)[i] == ' ') break;
                    }
                    break;
                case KEY_BACKSPACE:
                case 127:                                  /* Backspace */
                    if (i > 0) {
                        (*str)[i--] = '\0';
                        mvwprintw(d->win_input, 0, i, " ");
                        wmove(d->win_input, 0, i);
                    }
                    break;
                default:                                  /* Regular character */
                    if (i < (block_len + j)) {
                        mvwprintw(d->win_input, 0, i, "%c", ch);
                        (*str)[i] = ch;
                        i++;
                    } else if (i >= (block_len + j)) {
                        break;
                    }
            }
        }
    }

    return i;
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
    const char *s = get_current_playing();
    if (s != NULL) 
        mvwprintw(Window_audio, 0, (d->w - strlen(s))/2, "%s", s); 
    wrefresh(Window_audio);
}

int main() {
    static struct display display;
    display = init_terminal_ui(&display);
    init_mpd_connection();

    int i = 0;
    while (1) {
        if (has_resized(&display)) {
            adjust_window_size(&display);
            adjust_window_size(&display);
        }

        display_ui_elements(&display);

        int x = freadline(&(display.input), 41, &display);
        if (x > 1) {
            wclear(Window_err); wrefresh(Window_err);
            process_input_command(display.input);
            memset(display.input, 0, x);
        }
        usleep(10000);
    }

    exit_clean(0, NULL);
}


