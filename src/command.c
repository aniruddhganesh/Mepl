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
#include "ui.h"
#include "print.h"

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


void cmd_print(Token_t ts)
{
    // TODO: Handle different types of print commands
    if (ts.count > 1) {
        ReportErrorRunningCmd;
        return;
    }

    display.ui_state = PRINT_QUEUE;
}

void cmd_cls(Token_t ts)
{
    InvalidArgumentsAfter(1);
    display.ui_state = PRINT_NONE;
}

struct Cmd_t cmds[] = {
    {    "exit",         cmd_exit_zero,     },
    {    "pause",        cmd_song_pause,    },
    {    "resume",       cmd_song_resume,   },
    {    "next",         cmd_song_next,     },
    {    "previous",     cmd_song_prev,     },
    {    "print",        cmd_print,         },
    {    "cls",          cmd_cls,           },
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
