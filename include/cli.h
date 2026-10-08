#ifndef CLI_H
#define CLI_H

#include <stddef.h>

#include "gpib_command.h"

typedef struct {
    const char *config_path;
    int interactive;
    int command_probe;
    int command_ms;
    int command_trylock;
    int command_send;
    send_request_t send_request;
} cli_options_t;

void print_usage(const char *program_name);
int parse_cli(int argc, char **argv, cli_options_t *options, char *error, size_t error_size);

#endif
