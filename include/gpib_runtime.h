#ifndef GPIB_RUNTIME_H
#define GPIB_RUNTIME_H

#include "atfs_api.h"
#include "config.h"
#include "gpib_test.h"

typedef struct {
    void *realtime_library_handle;
    void *preload_library_handles[MAX_PRELOAD_LIBRARIES];
    int preload_library_count;
    void *library_handle;
    utsc_gpib_open_fn open;
    utsc_gpib_close_fn close;
    utsc_gpib_send_data_fn send_data;
    utsc_gpib_recv_data_fn recv_data;
    utsc_gpib_recv_stb_fn recv_stb;
    utsc_gpib_recv_stb_all_fn recv_stb_all;
    utsc_gpib_wait_srq_fn wait_srq;
    utsc_gpib_clear_device_fn clear_device;
    utsc_gpib_get_error_message_fn get_error_message;
    utsc_rm_lock_fn rm_lock;
    utsc_rm_trylock_fn rm_trylock;
    utsc_rm_unlock_fn rm_unlock;
    utsc_rm_get_error_message_fn rm_get_error_message;
    utsc_program_name_set_fn program_name_set;
    int library_loaded;
    UTSC_Gpib device_handle;
    int device_opened;
    int resource_locked;
    char locked_resource_name[MAX_DEVICE_NAME_LENGTH];
} gpib_runtime_t;

typedef struct {
    int success;
    char stage[64];
    char message[256];
} operation_result_t;

void set_atfs_program_name(const char *program_name);
void set_operation_error(operation_result_t *result, const char *stage, const char *message, const gpib_runtime_t *runtime);
void set_operation_success(operation_result_t *result, const char *stage, const gpib_runtime_t *runtime);
const char *runtime_error_message(const gpib_runtime_t *runtime, const char *fallback);
void close_runtime(gpib_runtime_t *runtime);
void init_runtime(gpib_runtime_t *runtime);
int ensure_runtime_ready(gpib_runtime_t *runtime, const app_config_t *config, operation_result_t *result);
int clear_device(gpib_runtime_t *runtime, operation_result_t *result);
operation_result_t try_resource_lock(gpib_runtime_t *runtime, const app_config_t *config);
operation_result_t probe_device(gpib_runtime_t *runtime, const app_config_t *config);

#endif
