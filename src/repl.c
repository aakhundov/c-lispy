#include "repl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edit.h"
#include "env.h"
#include "eval.h"
#include "parse.h"
#include "value.h"

static const char* exit_commands[] = {"exit", "quit", "q"};
static const char* clear_commands[] = {"clear", "clr", "clrscr"};
static const char* env_commands[] = {"env", "environment"};

static const char** commands[] = {
    exit_commands,
    clear_commands,
    env_commands};

static const size_t command_sizes[] = {
    sizeof(exit_commands) / sizeof(char*),
    sizeof(clear_commands) / sizeof(char*),
    sizeof(env_commands) / sizeof(char*)};

typedef enum {
    COMMAND_EXIT = 0,
    COMMAND_CLEAR = 1,
    COMMAND_ENV = 2,
    COMMAND_OTHER = -1
} command_type;

static command_type get_command_type(char* line) {
    size_t num_command_types = sizeof(commands) / sizeof(char**);
    for (size_t i = 0; i < num_command_types; i++) {
        size_t num_commands = command_sizes[i];
        for (size_t j = 0; j < num_commands; j++) {
            if (strcmp(line, commands[i][j]) == 0) {
                return i;
            }
        }
    }

    return COMMAND_OTHER;
}

static void process_repl_command(parser* p, environment* env, char* input, char* output) {
    add_history(input);

    result r;
    if (parser_parse(p, input, &r)) {
        tree t = result_get_tree(&r);
        value* v = value_from_tree(&t);
        value* e = value_evaluate(v, env);

        value_to_str(e, output);
        printf("%s\n", output);

        value_dispose(e);
        value_dispose(v);
        result_dispose_tree(&r);
    } else {
        result_print_error(&r);
        result_dispose_error(&r);
    }
}

void run_repl(parser* p, environment* env) {
    puts("clisp version 0.0.1");
    puts("enter \"quit\" to quit\n");

    int stop = 0;
    char output[16384];

    while (!stop) {
        char* input = readline("clisp> ");

        switch (get_command_type(input)) {
            case COMMAND_EXIT:
                stop = 1;
                break;
            case COMMAND_CLEAR:
                printf("\e[1;1H\e[2J");
                break;
            case COMMAND_ENV:
                environment_to_str(env, output);
                printf("%s", output);
                break;
            default:
                process_repl_command(p, env, input, output);
        }

        free(input);
    }

    printf("\nbye!\n");
}
