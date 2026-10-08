#include "interactive.h"

#include <stdio.h>
#include <string.h>

#include "gpib_command.h"
#include "gpib_runtime.h"
#include "gpib_test.h"
#include "output.h"
#include "shutdown.h"
#include "text_utils.h"

typedef struct {
    int choice;
    const char *menu_text;
    const char *command;
    read_mode_t read_mode;
} fixed_query_t;

static const fixed_query_t fixed_queries[] = {
    {2, "Send ms - Query Prober status", "ms", READ_PAYLOAD_ONLY},
    {3, "Send B - Query Prober ID", "B", READ_PAYLOAD_ONLY},
    {4, "Send V - Query Lot Number", "V", READ_PAYLOAD_ONLY},
    {5, "Send b - Query Wafer ID", "b", READ_PAYLOAD_ONLY},
    {6, "Send f - Query Chuck Temperature", "f", READ_PAYLOAD_ONLY},
    {7, "Send w - Query Wafer Status", "w", READ_PAYLOAD_ONLY},
    {8, "Send x - Query Cassette Status", "x", READ_PAYLOAD_ONLY},
    {9, "Send r - Query Hot-chuck Status", "r", READ_PAYLOAD_ONLY}
};

static int read_console_line(const char *prompt, char *buffer, size_t size) {
    if (prompt != NULL) {
        printf("%s", prompt);
        fflush(stdout);
    }
    if (is_interrupted()) {
        return 0;
    }
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }
    trim_in_place(buffer);
    return 1;
}

static int parse_menu_choice(const char *text, int *choice) {
    int value;
    if (!parse_int_value(text, &value)) {
        return 0;
    }
    *choice = value;
    return 1;
}

static void print_menu(void) {
    size_t index;

    printf("\n");
    printf("1. Probe Library And Device\n");
    for (index = 0; index < sizeof(fixed_queries) / sizeof(fixed_queries[0]); ++index) {
        printf("%d. %s\n", fixed_queries[index].choice, fixed_queries[index].menu_text);
    }
    printf("10. Send Custom Command - Read STB only\n");
    printf("11. Send Custom Command And Read Payload\n");
    printf("12. Send Custom Command - Wait SRQ, Read STB All And Payload\n");
    printf("13. Clear Device (UTSC_Gpib_ClearDevice)\n");
    printf("14. Show Current Config\n");
    printf("15. Try RM Lock Only (UTSC_Rm_Trylock)\n");
    printf("16. Exit\n");
}

static int send_fixed_query_if_matched(int choice, gpib_runtime_t *runtime, const app_config_t *config) {
    size_t index;

    for (index = 0; index < sizeof(fixed_queries) / sizeof(fixed_queries[0]); ++index) {
        if (fixed_queries[index].choice == choice) {
            gpib_reply_t reply = send_gpib_command(runtime,
                                                   config,
                                                   fixed_queries[index].command,
                                                   "",
                                                   fixed_queries[index].read_mode);
            print_reply(&reply);
            return 1;
        }
    }

    return 0;
}

static int build_custom_request(send_request_t *request, read_mode_t read_mode) {
    char command[MAX_COMMAND_LENGTH];
    char payload[MAX_PAYLOAD_LENGTH + 1];

    memset(request, 0, sizeof(*request));
    if (!read_console_line("Command: ", command, sizeof(command))) {
        return 0;
    }
    if (command[0] == '\0') {
        printf("Command is required.\n");
        return 0;
    }
    if (!is_ascii_text(command)) {
        printf("Command must be ASCII.\n");
        return 0;
    }
    if (!copy_string(request->command, sizeof(request->command), command)) {
        printf("Command is too long.\n");
        return 0;
    }

    if (!read_console_line("Payload (optional): ", payload, sizeof(payload))) {
        return 0;
    }
    if (!is_ascii_text(payload)) {
        printf("Payload must be ASCII.\n");
        return 0;
    }
    if (!copy_string(request->payload, sizeof(request->payload), payload)) {
        printf("Payload is too long.\n");
        return 0;
    }

    request->read_mode = read_mode;
    return 1;
}

int run_interactive(const app_config_t *config, const char *config_path) {
    gpib_runtime_t runtime;
    char line[64];
    int choice;

    init_runtime(&runtime);
    print_config(config, config_path);

    for (;;) {
        operation_result_t probe_result;
        gpib_reply_t reply;
        send_request_t request;

        if (is_interrupted()) {
            print_interrupted_message();
            close_runtime(&runtime);
            return INTERRUPTED_EXIT_CODE;
        }

        print_menu();
        if (!read_console_line("Choice: ", line, sizeof(line))) {
            if (is_interrupted()) {
                print_interrupted_message();
                close_runtime(&runtime);
                return INTERRUPTED_EXIT_CODE;
            }
            close_runtime(&runtime);
            return 1;
        }
        if (!parse_menu_choice(line, &choice)) {
            printf("Invalid choice.\n");
            continue;
        }

        if (choice == 1) {
            probe_result = probe_device(&runtime, config);
            print_operation_result(&probe_result);
            continue;
        }
        if (send_fixed_query_if_matched(choice, &runtime, config)) {
            continue;
        }
        if (choice == 10) {
            if (!build_custom_request(&request, READ_STB_ONLY)) {
                continue;
            }
            reply = send_gpib_command(&runtime, config, request.command, request.payload, request.read_mode);
            print_reply(&reply);
            continue;
        }
        if (choice == 11) {
            if (!build_custom_request(&request, READ_PAYLOAD_ONLY)) {
                continue;
            }
            reply = send_gpib_command(&runtime, config, request.command, request.payload, request.read_mode);
            print_reply(&reply);
            continue;
        }
        if (choice == 12) {
            if (!build_custom_request(&request, READ_WAIT_SRQ_STB_ALL_PAYLOAD)) {
                continue;
            }
            reply = send_gpib_command(&runtime, config, request.command, request.payload, request.read_mode);
            print_reply(&reply);
            continue;
        }
        if (choice == 13) {
            if (!ensure_runtime_ready(&runtime, config, &probe_result)) {
                print_operation_result(&probe_result);
                continue;
            }
            clear_device(&runtime, &probe_result);
            print_operation_result(&probe_result);
            continue;
        }
        if (choice == 14) {
            print_config(config, config_path);
            continue;
        }
        if (choice == 15) {
            probe_result = try_resource_lock(&runtime, config);
            print_operation_result(&probe_result);
            continue;
        }
        if (choice == 16) {
            close_runtime(&runtime);
            return 0;
        }

        printf("Choice must be between 1 and 16.\n");
    }
}
