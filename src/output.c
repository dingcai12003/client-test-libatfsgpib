#include "output.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

void trace_stage(const char *message) {
    printf("%s\n", message);
    fflush(stdout);
}

void print_operation_result(const operation_result_t *result) {
    printf("[%s] %s\n", result->stage, result->success ? "OK" : "FAILED");
    if (result->message[0] != '\0') {
        printf("Message: %s\n", result->message);
    }
}

void print_hex_bytes(const unsigned char *data, size_t length) {
    size_t index;
    for (index = 0; index < length; ++index) {
        printf("%02X", data[index]);
        if (index + 1 < length) {
            printf(" ");
        }
    }
}

void print_ascii_bytes(const unsigned char *data, size_t length) {
    size_t index;
    for (index = 0; index < length; ++index) {
        unsigned char value = data[index];
        if (value == '\r') {
            printf("\\r");
        } else if (value == '\n') {
            printf("\\n");
        } else if (value == '\t') {
            printf("\\t");
        } else if (isprint(value)) {
            printf("%c", value);
        } else {
            printf("\\x%02X", value);
        }
    }
}

void print_frame(const char *command, const char *payload) {
    char buffer[MAX_COMMAND_LENGTH + MAX_PAYLOAD_LENGTH + 4];
    size_t command_length = strlen(command);
    size_t payload_length = strlen(payload);
    size_t write_length = command_length + payload_length;

    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, command, command_length);
    memcpy(buffer + command_length, payload, payload_length);

    printf("Write ASCII: ");
    print_ascii_bytes((const unsigned char *)buffer, write_length);
    printf("\n");
    printf("Write HEX: ");
    print_hex_bytes((const unsigned char *)buffer, write_length);
    printf("\n");
}

void print_reply(const gpib_reply_t *reply) {
    printf("[%s] %s\n", reply->stage, reply->success ? "OK" : "FAILED");
    if (reply->message[0] != '\0') {
        printf("Message: %s\n", reply->message);
    }
    if (reply->stb_valid) {
        printf("STB: %d (0x%X)\n", reply->stb, (unsigned int)reply->stb);
    } else {
        printf("STB: unavailable\n");
    }
    if (reply->payload_valid) {
        size_t payload_length = strlen(reply->payload);
        printf("Payload ASCII: %s\n", reply->payload);
        printf("Payload HEX: ");
        print_hex_bytes((const unsigned char *)reply->payload, payload_length);
        printf("\n");
    } else {
        printf("Payload: unavailable\n");
    }
}

void print_config(const app_config_t *config, const char *path) {
    printf("Config file: %s\n", path);
    printf("library_path=%s\n", config->library_path);
    printf("library_bind_mode=%s\n", library_bind_mode_name(config->library_bind_mode));
    printf("preload_libraries=%s\n", config->preload_libraries);
    printf("device_name=%s\n", config->device_name);
    printf("read_buffer_size=%d\n", config->read_buffer_size);
    printf("wait_srq_timeout_us=%d\n", config->wait_srq_timeout_us);
    printf("use_rm_lock=%d\n", config->use_rm_lock);
}
