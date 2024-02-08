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
#include "ui.h"

#define UREFRESH 80000
#define CTRL_KEY(c) ((c) & 037)

struct display display;

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
    /*         Name             fg              bg   */
    init_pair(COL_CMD,      COLOR_BLACK,    COLOR_GREEN);
    init_pair(COL_AUD,      COLOR_BLUE,     COLOR_BLACK);
    init_pair(COL_UI,       COLOR_GREEN,    COLOR_BLACK);
    init_pair(COL_UI_ALT,   COLOR_BLUE,     COLOR_BLACK);
    init_pair(COL_PROG,     COLOR_MAGENTA,  COLOR_BLACK);
    init_pair(COL_INFO,     COLOR_BLUE,     COLOR_BLACK);
    init_pair(COL_ERR,      COLOR_RED,      COLOR_BLACK);
}


static void adjust_window_size(void)
{
    /* ----------------  2
     * |---audio------|
     * |              | 
     * |   win_ui     |  h - 3
     * |              |
     * |---input------| 1
     * =====err======== 1   */ 
    wclear(display.win_audio);
    wclear(display.win_input);
    wclear(display.win_ui);
    wclear(display.win_err);

    getmaxyx(stdscr, display.h, display.w);

    mvwin(display.win_audio, 0, 0);
    mvwin(display.win_ui, 2, 0);
    mvwin(display.win_input, display.h - 2, 0);
    mvwin(display.win_err, display.h - 1, 0);

    wresize(display.win_audio, 2 , display.w);
    wresize(display.win_ui, display.h - 4, display.w);
    wresize(display.win_input, 1, display.w);
    wresize(display.win_err, 1, display.w);

    box(display.win_ui, 0, 0);

    ui_wprint_blank_line(display.win_input, display.w, COL_CMD);

    wrefresh(display.win_audio);
    wrefresh(display.win_ui);
    wrefresh(display.win_err);
}


static struct display init_terminal_ui(void)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, true);
    initialize_colors();

    display.win_ui = newwin(0,0,0,0);
    display.win_input = newwin(0,0,0,0);
    display.win_err = newwin(0,0,0,0);
    display.win_audio = newwin(0,0,0,0);

    display.win_ui = display.win_ui;
    display.win_err = display.win_err;
    display.win_audio = display.win_audio;
    timeout(0);

    return display;
}

static bool has_resized(void)
{
    int h, w;
    getmaxyx(stdscr, h, w);
    return (display.w != w || display.h != h);
}

static bool has_changed_state(struct display *old_state)
{
    return old_state->ui_state == display.ui_state;
}



static void display_ui_elements(void)
{
    wclear(display.win_audio);
    char *cur_song_name = getstr_current_playing(MPD_TAG_TITLE);
    char *volume_str = getstr_volume();

    wattron(display.win_audio, COLOR_PAIR(COL_AUD));

    if (cur_song_name) {
        int align_middle = (display.w - strlen(cur_song_name)) / 2;
        mvwprintw(display.win_audio, 0, align_middle, "%s", cur_song_name); 
        free(cur_song_name);
    }

    if (volume_str)  {
        mvwprintw(display.win_audio, 0, 0, "%s", volume_str);
        free(volume_str);
    }

    unsigned elaps;
    unsigned dur;
    if (get_song_position_on_duration(&elaps, &dur)) {
        // "[mm:ss/mm:ss]" -> 13
        mvwprintw(display.win_audio, 0, display.w - 13, "[%.2d:%.2d/%.2d:%.2d]", 
                elaps / 60, elaps % 60, dur / 60, dur % 60); 
        // Progress bar
        wattron(display.win_audio, COLOR_PAIR(COL_PROG));
        unsigned perc_elaps = ((float)elaps/(float)dur) * display.w;

        for (int i = 0; i < display.w; i++) {
            if (i < perc_elaps) {
                mvwprintw(display.win_audio, 1, i, "=");
            } else if (i == perc_elaps) {
                mvwprintw(display.win_audio, 1, i, ">");
            } else {
                mvwprintw(display.win_audio, 1, i, "-");
            }
        }
        wattroff(display.win_audio, COLOR_PAIR(COL_PROG));
    }

    wrefresh(display.win_input);
    wrefresh(display.win_audio);
}

int main() {
    display = init_terminal_ui();
    init_mpd_connection();
    static struct display old_state;

    while (1) {
        if (has_resized()) {
            adjust_window_size();
            adjust_window_size();
        }


        int x = freadline(&display, 50);

        if (x > 0) {
            wclear(display.win_err); wrefresh(display.win_err);

            process_input_command(display.input);
            display.input_len = 0;
            memset(display.input, 0, x);
        }

        display_ui_elements();
        usleep(UREFRESH);
    }

    exit_clean(0, NULL);
}


