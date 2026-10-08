#include "gpib_command.h"

#include <stdio.h>
#include <string.h>

#include "output.h"
#include "text_utils.h"

static void set_reply_error(gpib_reply_t *reply, const char *stage, const char *message) {
    copy_string(reply->stage, sizeof(reply->stage), stage);
    copy_string(reply->message, sizeof(reply->message), message);
}

gpib_reply_t send_gpib_command(gpib_runtime_t *runtime,
                               const app_config_t *config,
                               const char *command,
                               const char *payload,
                               read_mode_t read_mode) {
    gpib_reply_t reply;
    operation_result_t operation;
    char write_buffer[MAX_COMMAND_LENGTH + MAX_PAYLOAD_LENGTH + 4];
    size_t command_length;
    size_t payload_length;
    UTSC_size_t write_length;
    int write_status;
    int status_read;
    int payload_status;

    memset(&reply, 0, sizeof(reply));

    if (!is_ascii_text(command) || !is_ascii_text(payload)) {
        set_reply_error(&reply, "validate-input", "command and payload must be ASCII");
        return reply;
    }

    command_length = strlen(command);
    payload_length = strlen(payload);
    write_length = (UTSC_size_t)(command_length + payload_length);

    if (command_length == 0 || command_length >= MAX_COMMAND_LENGTH) {
        set_reply_error(&reply, "validate-input", "command length is invalid");
        return reply;
    }

    if (payload_length >= MAX_PAYLOAD_LENGTH) {
        set_reply_error(&reply, "validate-input", "payload is too long");
        return reply;
    }

    if (!ensure_runtime_ready(runtime, config, &operation)) {
        set_reply_error(&reply, operation.stage, operation.message);
        return reply;
    }

    memset(write_buffer, 0, sizeof(write_buffer));
    memcpy(write_buffer, command, command_length);
    memcpy(write_buffer + command_length, payload, payload_length);

    print_frame(command, payload);

    write_status = runtime->send_data(runtime->device_handle,
                                      write_buffer,
                                      write_length);
    if (write_status != 0) {
        set_reply_error(&reply,
                        "write-command",
                        runtime_error_message(runtime, "GPIB write failed"));
        return reply;
    }

    if (read_mode == READ_WAIT_SRQ_STB_ALL_PAYLOAD) {
        int wait_status;
        int stb = 0;
        char trace_message[128];

        snprintf(trace_message,
                 sizeof(trace_message),
                 "wait-srq: before UTSC_Gpib_WaitSrq timeout_us=%d",
                 config->wait_srq_timeout_us);
        trace_stage(trace_message);
        wait_status = runtime->wait_srq(runtime->device_handle,
                                        (long)config->wait_srq_timeout_us);
        snprintf(trace_message,
                 sizeof(trace_message),
                 "wait-srq: after UTSC_Gpib_WaitSrq status=%d",
                 wait_status);
        trace_stage(trace_message);
        if (wait_status != 0) {
            set_reply_error(&reply,
                            "wait-srq",
                            runtime_error_message(runtime, "GPIB wait SRQ failed"));
            return reply;
        }

        trace_stage("read-stb-all: before UTSC_Gpib_RecvStbAll");
        status_read = runtime->recv_stb_all(runtime->device_handle, &stb);
        snprintf(trace_message,
                 sizeof(trace_message),
                 "read-stb-all: after UTSC_Gpib_RecvStbAll status=%d stb=%d",
                 status_read,
                 stb);
        trace_stage(trace_message);
        if (status_read != 0) {
            set_reply_error(&reply,
                            "read-stb-all",
                            runtime_error_message(runtime, "GPIB status byte queue read failed"));
            return reply;
        }
        reply.stb = stb;
        reply.stb_valid = 1;
    }

    if (read_mode == READ_STB_ONLY || read_mode == READ_STB_THEN_PAYLOAD) {
        int stb = 0;
        status_read = runtime->recv_stb(runtime->device_handle, &stb);
        if (status_read != 0) {
            set_reply_error(&reply,
                            "read-stb",
                            runtime_error_message(runtime, "GPIB status byte read failed"));
            return reply;
        }
        reply.stb = stb;
        reply.stb_valid = 1;
    }

    if (read_mode == READ_PAYLOAD_ONLY ||
        read_mode == READ_STB_THEN_PAYLOAD ||
        read_mode == READ_WAIT_SRQ_STB_ALL_PAYLOAD) {
        char payload_buffer[MAX_PAYLOAD_LENGTH + 1];
        UTSC_size_t actual_size = 0;
        memset(payload_buffer, 0, sizeof(payload_buffer));
        payload_status = runtime->recv_data(runtime->device_handle,
                                            payload_buffer,
                                            (UTSC_size_t)config->read_buffer_size,
                                            &actual_size);
        if (payload_status != 0) {
            set_reply_error(&reply,
                            "read-payload",
                            runtime_error_message(runtime, "GPIB payload read failed"));
            return reply;
        }

        if (actual_size > MAX_PAYLOAD_LENGTH) {
            actual_size = MAX_PAYLOAD_LENGTH;
        }
        payload_buffer[actual_size] = '\0';
        while (actual_size > 0) {
            char last = payload_buffer[actual_size - 1];
            if (last == '\0' || last == '\r' || last == '\n') {
                payload_buffer[actual_size - 1] = '\0';
                actual_size--;
                continue;
            }
            break;
        }

        copy_string(reply.payload, sizeof(reply.payload), payload_buffer);
        reply.payload_valid = 1;
    }

    reply.success = 1;
    if (read_mode == READ_STB_ONLY) {
        copy_string(reply.stage, sizeof(reply.stage), "read-stb");
    } else if (read_mode == READ_WAIT_SRQ_STB_ALL_PAYLOAD) {
        copy_string(reply.stage, sizeof(reply.stage), "wait-srq-read-payload");
    } else {
        copy_string(reply.stage, sizeof(reply.stage), "read-payload");
    }
    copy_string(reply.message, sizeof(reply.message), "OK");
    return reply;
}
