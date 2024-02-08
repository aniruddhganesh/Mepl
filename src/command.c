#include <curses.h>
#include <mpd/song.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpd/client.h>
#include <unistd.h>

#include "command.h"
#include "client_mpd.h"
#include "main.h"

#define InvalidArgumentsAfter(x) if (ts.count > x) {                \
    ui_print_error(COL_INFO, "Invalid Argument: %s", ts.tokens[x]); \
    return; }

#define ReportErrorRunningCmd ui_print_error(COL_ERR, "ERR: Cannot run '%s'", ts.tokens[0]);

/* Dumb public commands */
void cmd_exit_zero(Token_t ts)
{
    InvalidArgumentsAfter(1);
    exit_clean(0, NULL);
}

void cmd_song_pause(Token_t ts)
{
    InvalidArgumentsAfter(1);
    if (!mpd_run_pause(conn, true))
        ReportErrorRunningCmd;
}

void cmd_song_resume(Token_t ts)
{
    InvalidArgumentsAfter(1);
    if (!mpd_run_play(conn))
        ReportErrorRunningCmd
}

void cmd_song_next(Token_t ts)
{
    InvalidArgumentsAfter(1);
    if (!mpd_run_next(conn)) {
        mpd_connection_clear_error(conn);
        ReportErrorRunningCmd;
    }
}

void cmd_song_prev(Token_t ts)
{
    InvalidArgumentsAfter(1);
    if (!mpd_run_previous(conn)) {
        mpd_connection_clear_error(conn);
        ReportErrorRunningCmd;
    }
}

static void print_queue(struct mpd_song **queue)
{
    char *song_title = NULL;
    char *song_artist = NULL ;

    char *current_song_title = getstr_current_playing(MPD_TAG_TITLE);

    ui_print_str(true, "\r");

    for (size_t i = 0; queue[i] != NULL; i++) {
        song_title = getstr_song_info(queue[i], MPD_TAG_TITLE);
        song_artist = getstr_song_info(queue[i], MPD_TAG_ARTIST);

        if (!song_title || !song_artist) {
            ui_print_error(COL_ERR, "Unable to get song info");
            return;
        }

        char *song_title_c = utf8_to_cstr(song_title);
        char *song_artist_c = utf8_to_cstr(song_artist);

    
        if (!strcmp(current_song_title, song_title)) {
            wattron(Window_ui, COLOR_PAIR(COL_UI_ALT));
            ui_print_str(false, "%s", song_title_c);
        } else {
            wattron(Window_ui, COLOR_PAIR(COL_UI));
            ui_print_str(false, "%s", song_title_c);
        }


        free(song_title);
        free(song_artist);
        free(song_title_c);
        free(song_artist_c);
    }
}

void cmd_handle_print(Token_t ts)
{
    // TODO: Handle different types of print commands
    if (ts.count > 1) {
        ReportErrorRunningCmd;
        return;
    }

    struct mpd_song **queue = get_song_queue();
    
    if (!queue) { 
        ReportErrorRunningCmd;
        return;
    }
    print_queue(queue);
    free(queue);
}

struct Cmd_t cmds[] = {
    {    "exit",         cmd_exit_zero,     },
    {    "pause",        cmd_song_pause,    },
    {    "resume",       cmd_song_resume,   },
    {    "next",         cmd_song_next,     },
    {    "previous",     cmd_song_prev,     },
    {    "print",        cmd_handle_print,  },
    {    NULL,           NULL,              },
};

/* Tokenisation and Processing */
static Token_t tokenize_command(const char *str)
{
    Token_t result;

    /* Worst case scenario */
    result.tokens = malloc(sizeof(const char *) * strlen(str));
    result.length = malloc(sizeof(unsigned)     * strlen(str));
    result.count = 1;

    result.tokens[0] = strtok((char *)str, " ");

    char *token = NULL;

    for (size_t i = 1;; i++) {
        token = strtok(NULL, " ");
        if (token == NULL) break;

        result.tokens[i] = strdup(token);
        result.length[i] = strlen(token);
        result.count++;
    }

    result.tokens = realloc(result.tokens, sizeof(const char*) *result.count);
    result.length = realloc(result.length, sizeof(const char*) *result.count);

    return result;
}

// Double free protected !!
static void free_tokens(Token_t *t)
{
    if (t->count) {
        free(t->length);
        free(t->tokens);
        t->count = 0;
    }
}

void process_input_command(const char *input)
{
    Token_t tok_in = tokenize_command(input);
    
    for (size_t i = 0; cmds[i].name != NULL; i++) {
        if (!strcmp(tok_in.tokens[0], cmds[i].name)) {
            cmds[i].func(tok_in);
            free_tokens(&tok_in);
            return;
        }
    }

    free_tokens(&tok_in);
    ui_print_error(COL_ERR, "No Such Command: %s", input);
    return;
}
