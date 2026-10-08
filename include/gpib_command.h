#ifndef GPIB_COMMAND_H
#define GPIB_COMMAND_H

#include "gpib_runtime.h"

typedef enum {
    READ_STB_ONLY = 0,
    READ_PAYLOAD_ONLY = 1,
    READ_STB_THEN_PAYLOAD = 2,
    READ_WAIT_SRQ_STB_ALL_PAYLOAD = 3
} read_mode_t;

typedef struct {
    int success;
    int stb;
    int stb_valid;
    char payload[MAX_PAYLOAD_LENGTH + 1];
    int payload_valid;
    char stage[64];
    char message[256];
} gpib_reply_t;

typedef struct {
    char command[MAX_COMMAND_LENGTH];
    char payload[MAX_PAYLOAD_LENGTH + 1];
    read_mode_t read_mode;
} send_request_t;

gpib_reply_t send_gpib_command(gpib_runtime_t *runtime,
                               const app_config_t *config,
                               const char *command,
                               const char *payload,
                               read_mode_t read_mode);

#endif
