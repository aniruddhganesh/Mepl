#ifndef COMMAND_H
#define COMMAND_H

typedef struct Token_t {
    char **tokens;
    unsigned *length;
    unsigned count;
} Token_t;

struct Cmd_t {
    char *name;
    bool (*func)(Token_t ts);
};

extern struct Cmd_t cmds[];

void process_input_command(const char *input);

#endif
