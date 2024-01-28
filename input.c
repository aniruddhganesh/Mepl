#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"

#define CTRL_KEY(c) ((c) & 037)

static void input_insert_char(struct display *d, char ch)
{
    d->input[d->input_len] = ch;
    mvwprintw(d->win_input, 0, d->input_len, "%c", ch);
    d->input_len++;
}

static void input_insert_newline(struct display *d)
{
    wclear(d->win_input);
    wprint_blank_line(d->win_input, d->w, COL_CMD);
    d->input[d->input_len++] = '\0';
}

static void input_backspace(struct display *d)
{
    d->input[d->input_len--] = '\0';
    mvwprintw(d->win_input, 0, d->input_len, " ");
    wmove(d->win_input, 0, d->input_len);
}

int freadline(struct display *d, const size_t block_len)
{
    int ch = '\0';
    size_t cur_block = 0;

    for (cur_block = block_len ;; cur_block += block_len) {
        cur_block += block_len;
        d->input = realloc(d->input, cur_block);

        while (d->input_len < cur_block && (ch = wgetch(d->win_input))) {
            switch (ch) {
                case '\n':
                    input_insert_newline(d);
                    return d->input_len;
                case CTRL_KEY('w'):
                    while (d->input_len > 0) {
                        input_backspace(d);
                        if (d->input[d->input_len] == ' ') break;
                    }
                    break;
                case KEY_BACKSPACE: case 127:
                    if (d->input_len > 0)
                        input_backspace(d);
                    break;
                default:
                    input_insert_char(d, ch);
            }
        }
    }

    return d->input_len;
}
