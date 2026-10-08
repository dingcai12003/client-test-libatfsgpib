#ifndef OUTPUT_H
#define OUTPUT_H

#include <stddef.h>

#include "config.h"
#include "gpib_command.h"
#include "gpib_runtime.h"

void trace_stage(const char *message);
void print_operation_result(const operation_result_t *result);
void print_hex_bytes(const unsigned char *data, size_t length);
void print_ascii_bytes(const unsigned char *data, size_t length);
void print_frame(const char *command, const char *payload);
void print_reply(const gpib_reply_t *reply);
void print_config(const app_config_t *config, const char *path);

#endif
