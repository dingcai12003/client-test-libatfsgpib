#include "cli.h"

#include <stdio.h>
#include <string.h>

#include "text_utils.h"

void print_usage(const char *program_name) {
    printf("Usage:\n");
    printf("  %s --config ./config/gpib-test.conf\n", program_name);
    printf("  %s --config ./config/gpib-test.conf probe\n", program_name);
    printf("  %s --config ./config/gpib-test.conf ms\n", program_name);
    printf("  %s --config ./config/gpib-test.conf trylock\n", program_name);
    printf("  %s --config ./config/gpib-test.conf send --command \"<cmd>\" [--payload \"<text>\"] [--read-payload]\n", program_name);
}

int parse_cli(int argc, char **argv, cli_options_t *options, char *error, size_t error_size) {
    int index = 1;

    memset(options, 0, sizeof(*options));
    options->config_path = "./config/gpib-test.conf";
    options->interactive = 1;

    while (index < argc) {
        if (strcmp(argv[index], "--config") == 0) {
            if (index + 1 >= argc) {
                snprintf(error, error_size, "--config requires a path");
                return 0;
            }
            options->config_path = argv[index + 1];
            index += 2;
            continue;
        }
        break;
    }

    if (index >= argc) {
        return 1;
    }

    options->interactive = 0;

    if (strcmp(argv[index], "probe") == 0) {
        options->command_probe = 1;
        if (index + 1 != argc) {
            snprintf(error, error_size, "probe does not accept extra arguments");
            return 0;
        }
        return 1;
    }

    if (strcmp(argv[index], "ms") == 0) {
        options->command_ms = 1;
        if (index + 1 != argc) {
            snprintf(error, error_size, "ms does not accept extra arguments");
            return 0;
        }
        return 1;
    }

    if (strcmp(argv[index], "trylock") == 0) {
        options->command_trylock = 1;
        if (index + 1 != argc) {
            snprintf(error, error_size, "trylock does not accept extra arguments");
            return 0;
        }
        return 1;
    }

    if (strcmp(argv[index], "send") == 0) {
        int saw_command = 0;
        options->command_send = 1;
        index++;
        while (index < argc) {
            if (strcmp(argv[index], "--command") == 0) {
                if (index + 1 >= argc) {
                    snprintf(error, error_size, "--command requires a value");
                    return 0;
                }
                if (!copy_string(options->send_request.command, sizeof(options->send_request.command), argv[index + 1])) {
                    snprintf(error, error_size, "command is too long");
                    return 0;
                }
                saw_command = 1;
                index += 2;
                continue;
            }
            if (strcmp(argv[index], "--payload") == 0) {
                if (index + 1 >= argc) {
                    snprintf(error, error_size, "--payload requires a value");
                    return 0;
                }
                if (!copy_string(options->send_request.payload, sizeof(options->send_request.payload), argv[index + 1])) {
                    snprintf(error, error_size, "payload is too long");
                    return 0;
                }
                index += 2;
                continue;
            }
            if (strcmp(argv[index], "--read-payload") == 0) {
                options->send_request.read_mode = READ_PAYLOAD_ONLY;
                index++;
                continue;
            }
            snprintf(error, error_size, "Unknown argument: %s", argv[index]);
            return 0;
        }

        if (!saw_command) {
            snprintf(error, error_size, "send requires --command");
            return 0;
        }
        if (!is_ascii_text(options->send_request.command) || !is_ascii_text(options->send_request.payload)) {
            snprintf(error, error_size, "command and payload must be ASCII");
            return 0;
        }
        return 1;
    }

    snprintf(error, error_size, "Unknown command: %s", argv[index]);
    return 0;
}
