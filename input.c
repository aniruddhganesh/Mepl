#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ncurses.h>

#include "main.h"
#include "command.h"

#define CTRL_KEY(c) ((c) & 037)

static int auto_complete_guess(const char *input, void **search_arr, size_t nsize)
{
    int idx = -10;
    bool first_occurance = true;

    for (int i = 0; *(search_arr + i) != NULL; i += nsize/8) {
        char *s = (char *)(*(search_arr +i));
        if (!strncmp(input, s, strlen(input))) {
            idx = (first_occurance) ? i/8 : -10;
            first_occurance = false;
        }
    }

    return idx;
}

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

static void input_autocomplete(struct display *d, size_t cur_block)
{
    bool is_command = (strchr(d->input, ' ')) ? true : false;
    void *search_arr = (is_command) ? NULL  : cmds;
    size_t nsize_arr = (is_command) ? 0     : sizeof(*cmds);

    // Temporary
    if (!search_arr) return;

    int byte_index = auto_complete_guess(d->input, search_arr, nsize_arr);
    if (byte_index < 0) return;

    char *autocmp_str = *((char **)search_arr + byte_index);
    int autocmp_strlen = strlen(autocmp_str);

    if (autocmp_strlen > cur_block) return;

    strcat(d->input, autocmp_str + d->input_len);
    mvwprintw(d->win_input, 0, d->input_len, "%s", autocmp_str + d->input_len);
    free(autocmp_str);

    d->input_len += (autocmp_strlen - d->input_len);
}

int freadline(struct display *d, const size_t block_len)
{
    int ch = '\0';
    size_t cur_block = 0;

    nodelay(d->win_input, true);

    for (cur_block = block_len ;; cur_block += block_len) {
        cur_block += block_len;
        d->input = realloc(d->input, cur_block);

        while ((ch = wgetch(d->win_input)) &&  d->input_len < cur_block) {
            switch (ch) {
                case ERR:
                    return -1;
                case '\n':
                    input_insert_newline(d);
                    return d->input_len;
                case '\t':
                    input_autocomplete(d, cur_block);
                    break;
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
