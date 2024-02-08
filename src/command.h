#ifndef COMMAND_H
#define COMMAND_H

typedef struct Token_t {
    char **tokens;
    unsigned *length;
    unsigned count;
} Token_t;

struct Cmd_t {
    char *name;
    void (*func)(Token_t ts);
};

enum PrintCmds {
    PRINT_QUEUE = 1,
};

extern struct Cmd_t cmds[];

void process_input_command(const char *input);

#endif
