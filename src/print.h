#ifndef PRINT_H
#define PRINT_H

enum Print_State {
    PRINT_NONE = 0,
    PRINT_QUEUE,
};

void print_run_print_state(enum Print_State ps);

#endif
