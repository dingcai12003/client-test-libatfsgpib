#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cli.h"
#include "config.h"
#include "gpib_command.h"
#include "gpib_runtime.h"
#include "interactive.h"
#include "output.h"
#include "shutdown.h"

static int is_legacy_config_path(const char *path) {
    return path != NULL &&
           (strcmp(path, "./gpib-test.conf") == 0 ||
            strcmp(path, "gpib-test.conf") == 0);
}

static int file_exists(const char *path) {
    return path != NULL && access(path, F_OK) == 0;
}

static int finish_runtime_command(gpib_runtime_t *runtime, int success) {
    close_runtime(runtime);
    if (is_interrupted()) {
        print_interrupted_message();
        return INTERRUPTED_EXIT_CODE;
    }
    return success ? 0 : 1;
}

int main(int argc, char **argv) {
    cli_options_t options;
    app_config_t config;
    gpib_runtime_t runtime;
    operation_result_t probe_result;
    gpib_reply_t reply;
    const char *active_config_path;
    char error[256];

    memset(error, 0, sizeof(error));
    if (argc > 0 && argv[0] != NULL && argv[0][0] != '\0') {
        set_atfs_program_name(argv[0]);
    }
    install_signal_handlers();

    if (!parse_cli(argc, argv, &options, error, sizeof(error))) {
        fprintf(stderr, "Argument error: %s\n", error);
        print_usage(argv[0]);
        return 2;
    }

    active_config_path = options.config_path;
    if (!load_config_file(active_config_path, &config, error, sizeof(error))) {
        if (is_legacy_config_path(options.config_path) &&
            !file_exists(options.config_path) &&
            load_config_file("./config/gpib-test.conf", &config, error, sizeof(error))) {
            active_config_path = "./config/gpib-test.conf";
        } else {
            fprintf(stderr, "Config error: %s\n", error);
            return 2;
        }
    }

    if (options.interactive) {
        return run_interactive(&config, active_config_path);
    }

    init_runtime(&runtime);
    print_config(&config, active_config_path);

    if (options.command_probe) {
        probe_result = probe_device(&runtime, &config);
        print_operation_result(&probe_result);
        return finish_runtime_command(&runtime, probe_result.success);
    }

    if (options.command_ms) {
        reply = send_gpib_command(&runtime, &config, "ms", "", READ_PAYLOAD_ONLY);
        print_reply(&reply);
        return finish_runtime_command(&runtime, reply.success);
    }

    if (options.command_trylock) {
        probe_result = try_resource_lock(&runtime, &config);
        print_operation_result(&probe_result);
        return finish_runtime_command(&runtime, probe_result.success);
    }

    if (options.command_send) {
        reply = send_gpib_command(&runtime,
                                  &config,
                                  options.send_request.command,
                                  options.send_request.payload,
                                  options.send_request.read_mode);
        print_reply(&reply);
        return finish_runtime_command(&runtime, reply.success);
    }

    close_runtime(&runtime);
    print_usage(argv[0]);
    return 2;
}
