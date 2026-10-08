#include "gpib_runtime.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#include "output.h"
#include "text_utils.h"

typedef enum {
    LIBRARY_REQUIRE_GPIB = 0,
    LIBRARY_REQUIRE_RM_TRYLOCK = 1
} library_requirement_t;

typedef enum {
    RUNTIME_SYMBOL_GPIB_OPEN = 0,
    RUNTIME_SYMBOL_GPIB_CLOSE,
    RUNTIME_SYMBOL_GPIB_SEND_DATA,
    RUNTIME_SYMBOL_GPIB_RECV_DATA,
    RUNTIME_SYMBOL_GPIB_RECV_STB,
    RUNTIME_SYMBOL_GPIB_RECV_STB_ALL,
    RUNTIME_SYMBOL_GPIB_WAIT_SRQ,
    RUNTIME_SYMBOL_GPIB_CLEAR_DEVICE,
    RUNTIME_SYMBOL_GPIB_GET_ERROR_MESSAGE,
    RUNTIME_SYMBOL_RM_LOCK,
    RUNTIME_SYMBOL_RM_TRYLOCK,
    RUNTIME_SYMBOL_RM_UNLOCK,
    RUNTIME_SYMBOL_RM_GET_ERROR_MESSAGE,
    RUNTIME_SYMBOL_PROGRAM_NAME_SET
} runtime_symbol_id_t;

typedef struct {
    runtime_symbol_id_t id;
    const char *name;
} runtime_symbol_t;

static const char *program_name_for_atfs = "gpib-test";

void set_atfs_program_name(const char *program_name) {
    if (program_name != NULL && program_name[0] != '\0') {
        program_name_for_atfs = program_name;
    }
}

static const char *gpib_status_name(int status) {
    switch (status) {
        case 0:
            return "OK";
        case UT_ERR_SC_GPIB_TIMEOUT:
            return "UT_ERR_SC_GPIB_TIMEOUT";
        case UT_ERR_SC_GPIB_LEAVEDATA:
            return "UT_ERR_SC_GPIB_LEAVEDATA";
        case UT_ERR_SC_GPIB_NOTFINDBOARD:
            return "UT_ERR_SC_GPIB_NOTFINDBOARD";
        case UT_ERR_SC_GPIB_IGNOREDEVICE:
            return "UT_ERR_SC_GPIB_IGNOREDEVICE";
        case UT_ERR_SC_GPIB_ALREADYDEVICE:
            return "UT_ERR_SC_GPIB_ALREADYDEVICE";
        case UT_ERR_SC_GPIB_NOTFINDDEVICE:
            return "UT_ERR_SC_GPIB_NOTFINDDEVICE";
        case UT_ERR_SC_GPIB_NULLARGS:
            return "UT_ERR_SC_GPIB_NULLARGS";
        case UT_ERR_SC_GPIB_ENVNAME:
            return "UT_ERR_SC_GPIB_ENVNAME";
        case UT_ERR_SC_GPIB_SERVNOTRUN:
            return "UT_ERR_SC_GPIB_SERVNOTRUN";
        case UT_ERR_SC_GPIB_DESCINVALID:
            return "UT_ERR_SC_GPIB_DESCINVALID";
        case UT_SERR_SC_GPIB_NOCONN:
            return "UT_SERR_SC_GPIB_NOCONN";
        case UT_SERR_SC_GPIB_PSIZE:
            return "UT_SERR_SC_GPIB_PSIZE";
        case UT_SERR_SC_GPIB_GPIBIF:
            return "UT_SERR_SC_GPIB_GPIBIF";
        case UT_SERR_SC_GPIB_RECVSIZE:
            return "UT_SERR_SC_GPIB_RECVSIZE";
        case UT_SERR_SC_GPIB_UNDEF:
            return "UT_SERR_SC_GPIB_UNDEF";
        default:
            return "UNKNOWN_GPIB_STATUS";
    }
}

static int dlopen_flags_for_config(const app_config_t *config) {
    int bind_flag = config->library_bind_mode == LIBRARY_BIND_LAZY ? RTLD_LAZY : RTLD_NOW;
    return bind_flag | RTLD_GLOBAL;
}

void set_operation_error(operation_result_t *result, const char *stage, const char *message, const gpib_runtime_t *runtime) {
    (void)runtime;
    memset(result, 0, sizeof(*result));
    result->success = 0;
    copy_string(result->stage, sizeof(result->stage), stage);
    copy_string(result->message, sizeof(result->message), message);
}

void set_operation_success(operation_result_t *result, const char *stage, const gpib_runtime_t *runtime) {
    (void)runtime;
    memset(result, 0, sizeof(*result));
    result->success = 1;
    copy_string(result->stage, sizeof(result->stage), stage);
    copy_string(result->message, sizeof(result->message), "OK");
}

const char *runtime_error_message(const gpib_runtime_t *runtime, const char *fallback) {
    char *message;
    if (runtime != NULL && runtime->get_error_message != NULL) {
        message = runtime->get_error_message();
        if (message != NULL && message[0] != '\0') {
            return message;
        }
    }
    return fallback;
}

static const char *resource_error_message(const gpib_runtime_t *runtime, const char *fallback) {
    char *message;
    if (runtime != NULL && runtime->rm_get_error_message != NULL) {
        message = runtime->rm_get_error_message();
        if (message != NULL && message[0] != '\0') {
            return message;
        }
    }
    return fallback;
}

static void unlock_resource_if_needed(gpib_runtime_t *runtime) {
    char message[MAX_DEVICE_NAME_LENGTH + 128];
    int status;

    if (!runtime->resource_locked || runtime->rm_unlock == NULL) {
        return;
    }

    snprintf(message,
             sizeof(message),
             "resource-unlock: before UTSC_Rm_Unlock(%s)",
             runtime->locked_resource_name);
    trace_stage(message);
    status = runtime->rm_unlock(runtime->locked_resource_name);
    snprintf(message,
             sizeof(message),
             "resource-unlock: after UTSC_Rm_Unlock status=%d",
             status);
    trace_stage(message);

    runtime->resource_locked = 0;
    runtime->locked_resource_name[0] = '\0';
}

void close_runtime(gpib_runtime_t *runtime) {
    int index;
    if (runtime->device_opened && runtime->close != NULL) {
        runtime->close(runtime->device_handle);
        runtime->device_opened = 0;
    }
    unlock_resource_if_needed(runtime);
    if (runtime->library_handle != NULL) {
        dlclose(runtime->library_handle);
    }
    for (index = runtime->preload_library_count - 1; index >= 0; --index) {
        if (runtime->preload_library_handles[index] != NULL) {
            dlclose(runtime->preload_library_handles[index]);
        }
    }
    if (runtime->realtime_library_handle != NULL) {
        dlclose(runtime->realtime_library_handle);
    }
    memset(runtime, 0, sizeof(*runtime));
}

void init_runtime(gpib_runtime_t *runtime) {
    memset(runtime, 0, sizeof(*runtime));
}

static int preload_configured_libraries(gpib_runtime_t *runtime,
                                        const app_config_t *config,
                                        operation_result_t *result) {
    char libraries[MAX_LINE_LENGTH];
    char *token;
    int dlopen_flags;

    if (config->preload_libraries[0] == '\0') {
        return 1;
    }

    dlopen_flags = dlopen_flags_for_config(config);
    copy_string(libraries, sizeof(libraries), config->preload_libraries);
    token = strtok(libraries, ",");
    while (token != NULL) {
        char message[PATH_MAX + 128];
        void *handle;

        trim_in_place(token);
        if (token[0] == '\0') {
            token = strtok(NULL, ",");
            continue;
        }

        if (runtime->preload_library_count >= MAX_PRELOAD_LIBRARIES) {
            snprintf(message,
                     sizeof(message),
                     "preload_libraries supports at most %d entries",
                     MAX_PRELOAD_LIBRARIES);
            set_operation_error(result, "preload-library", message, runtime);
            return 0;
        }

        snprintf(message, sizeof(message), "preload-library: before dlopen %s", token);
        trace_stage(message);
        handle = dlopen(token, dlopen_flags);
        if (handle == NULL) {
            const char *error = dlerror();
            snprintf(message,
                     sizeof(message),
                     "Unable to preload %s: %s",
                     token,
                     error != NULL ? error : "unknown error");
            set_operation_error(result, "preload-library", message, runtime);
            return 0;
        }

        runtime->preload_library_handles[runtime->preload_library_count] = handle;
        runtime->preload_library_count++;
        snprintf(message, sizeof(message), "preload-library: after dlopen %s", token);
        trace_stage(message);

        token = strtok(NULL, ",");
    }

    return 1;
}

static int validate_runtime_symbols(gpib_runtime_t *runtime,
                                    const app_config_t *config,
                                    library_requirement_t requirement,
                                    operation_result_t *result) {
    if (requirement == LIBRARY_REQUIRE_RM_TRYLOCK) {
        if (runtime->rm_trylock == NULL || runtime->rm_unlock == NULL ||
            runtime->rm_get_error_message == NULL) {
            set_operation_error(result,
                                "resolve-rm-symbols",
                                "GPIB library is missing required ATFS RM TryLock symbols",
                                runtime);
            return 0;
        }
        return 1;
    }

    if (runtime->open == NULL || runtime->close == NULL || runtime->send_data == NULL ||
        runtime->recv_data == NULL || runtime->recv_stb == NULL ||
        runtime->recv_stb_all == NULL || runtime->wait_srq == NULL ||
        runtime->clear_device == NULL || runtime->get_error_message == NULL ||
        (config->use_rm_lock &&
         (runtime->rm_lock == NULL || runtime->rm_unlock == NULL ||
          runtime->rm_get_error_message == NULL))) {
        set_operation_error(result,
                            "resolve-symbols",
                            config->use_rm_lock ?
                                "GPIB library is missing required ATFS UTSC_Gpib or UTSC_Rm symbols" :
                                "GPIB library is missing required ATFS UTSC_Gpib symbols",
                            runtime);
        return 0;
    }

    return 1;
}

static void resolve_runtime_symbols(gpib_runtime_t *runtime) {
    size_t index;
    runtime_symbol_t symbols[] = {
        {RUNTIME_SYMBOL_GPIB_OPEN, "UTSC_Gpib_Open"},
        {RUNTIME_SYMBOL_GPIB_CLOSE, "UTSC_Gpib_Close"},
        {RUNTIME_SYMBOL_GPIB_SEND_DATA, "UTSC_Gpib_SendData"},
        {RUNTIME_SYMBOL_GPIB_RECV_DATA, "UTSC_Gpib_RecvData"},
        {RUNTIME_SYMBOL_GPIB_RECV_STB, "UTSC_Gpib_RecvStb"},
        {RUNTIME_SYMBOL_GPIB_RECV_STB_ALL, "UTSC_Gpib_RecvStbAll"},
        {RUNTIME_SYMBOL_GPIB_WAIT_SRQ, "UTSC_Gpib_WaitSrq"},
        {RUNTIME_SYMBOL_GPIB_CLEAR_DEVICE, "UTSC_Gpib_ClearDevice"},
        {RUNTIME_SYMBOL_GPIB_GET_ERROR_MESSAGE, "UTSC_Gpib_GetErrorMessage"},
        {RUNTIME_SYMBOL_RM_LOCK, "UTSC_Rm_Lock"},
        {RUNTIME_SYMBOL_RM_TRYLOCK, "UTSC_Rm_TryLock"},
        {RUNTIME_SYMBOL_RM_UNLOCK, "UTSC_Rm_Unlock"},
        {RUNTIME_SYMBOL_RM_GET_ERROR_MESSAGE, "UTSC_Rm_GetErrorMessage"},
        {RUNTIME_SYMBOL_PROGRAM_NAME_SET, "UTSC_ProgramName_Set"}
    };

    for (index = 0; index < sizeof(symbols) / sizeof(symbols[0]); ++index) {
        void *symbol = dlsym(runtime->library_handle, symbols[index].name);

        switch (symbols[index].id) {
            case RUNTIME_SYMBOL_GPIB_OPEN:
                runtime->open = (utsc_gpib_open_fn)symbol;
                break;
            case RUNTIME_SYMBOL_GPIB_CLOSE:
                runtime->close = (utsc_gpib_close_fn)symbol;
                break;
            case RUNTIME_SYMBOL_GPIB_SEND_DATA:
                runtime->send_data = (utsc_gpib_send_data_fn)symbol;
                break;
            case RUNTIME_SYMBOL_GPIB_RECV_DATA:
                runtime->recv_data = (utsc_gpib_recv_data_fn)symbol;
                break;
            case RUNTIME_SYMBOL_GPIB_RECV_STB:
                runtime->recv_stb = (utsc_gpib_recv_stb_fn)symbol;
                break;
            case RUNTIME_SYMBOL_GPIB_RECV_STB_ALL:
                runtime->recv_stb_all = (utsc_gpib_recv_stb_all_fn)symbol;
                break;
            case RUNTIME_SYMBOL_GPIB_WAIT_SRQ:
                runtime->wait_srq = (utsc_gpib_wait_srq_fn)symbol;
                break;
            case RUNTIME_SYMBOL_GPIB_CLEAR_DEVICE:
                runtime->clear_device = (utsc_gpib_clear_device_fn)symbol;
                break;
            case RUNTIME_SYMBOL_GPIB_GET_ERROR_MESSAGE:
                runtime->get_error_message = (utsc_gpib_get_error_message_fn)symbol;
                break;
            case RUNTIME_SYMBOL_RM_LOCK:
                runtime->rm_lock = (utsc_rm_lock_fn)symbol;
                break;
            case RUNTIME_SYMBOL_RM_TRYLOCK:
                runtime->rm_trylock = (utsc_rm_trylock_fn)symbol;
                break;
            case RUNTIME_SYMBOL_RM_UNLOCK:
                runtime->rm_unlock = (utsc_rm_unlock_fn)symbol;
                break;
            case RUNTIME_SYMBOL_RM_GET_ERROR_MESSAGE:
                runtime->rm_get_error_message = (utsc_rm_get_error_message_fn)symbol;
                break;
            case RUNTIME_SYMBOL_PROGRAM_NAME_SET:
                runtime->program_name_set = (utsc_program_name_set_fn)symbol;
                break;
        }
    }
}

static int load_library(gpib_runtime_t *runtime,
                        const app_config_t *config,
                        operation_result_t *result,
                        library_requirement_t requirement) {
    char message[PATH_MAX + 128];
    int dlopen_flags = dlopen_flags_for_config(config);

    if (runtime->library_loaded) {
        if (!validate_runtime_symbols(runtime, config, requirement, result)) {
            return 0;
        }
        set_operation_success(result, "load-library", runtime);
        return 1;
    }

    snprintf(message, sizeof(message), "load-library: bind mode=%s", library_bind_mode_name(config->library_bind_mode));
    trace_stage(message);

    trace_stage("load-library: before dlopen librt.so.1");
    runtime->realtime_library_handle = dlopen("librt.so.1", RTLD_NOW | RTLD_GLOBAL);
    if (runtime->realtime_library_handle == NULL) {
        const char *error = dlerror();
        printf("load-library: dlopen librt.so.1 failed: %s\n", error != NULL ? error : "unknown error");
        fflush(stdout);
    } else {
        trace_stage("load-library: after dlopen librt.so.1");
    }

    if (!preload_configured_libraries(runtime, config, result)) {
        close_runtime(runtime);
        return 0;
    }

    trace_stage("load-library: before dlopen");
    runtime->library_handle = dlopen(config->library_path, dlopen_flags);
    if (runtime->library_handle == NULL) {
        snprintf(message, sizeof(message), "Unable to load %s: %s", config->library_path, dlerror());
        set_operation_error(result, "load-library", message, runtime);
        close_runtime(runtime);
        return 0;
    }
    trace_stage("load-library: after dlopen");

    trace_stage("resolve-symbols: before dlsym");
    resolve_runtime_symbols(runtime);
    trace_stage("resolve-symbols: after dlsym");

    if (!validate_runtime_symbols(runtime, config, requirement, result)) {
        close_runtime(runtime);
        return 0;
    }

    if (runtime->program_name_set != NULL) {
        trace_stage("program-name: before UTSC_ProgramName_Set");
        runtime->program_name_set(program_name_for_atfs);
        trace_stage("program-name: after UTSC_ProgramName_Set");
    }

    runtime->library_loaded = 1;
    set_operation_success(result, "resolve-symbols", runtime);
    return 1;
}

static int open_device(gpib_runtime_t *runtime, const app_config_t *config, operation_result_t *result) {
    int status;
    char message[256];
    char locked_resource_name[MAX_DEVICE_NAME_LENGTH];

    if (runtime->device_opened) {
        set_operation_success(result, "open-device", runtime);
        return 1;
    }

    if (config->use_rm_lock && !runtime->resource_locked) {
        memset(locked_resource_name, 0, sizeof(locked_resource_name));
        snprintf(message,
                 sizeof(message),
                 "resource-lock: before UTSC_Rm_Lock(%s)",
                 config->device_name);
        trace_stage(message);
        status = runtime->rm_lock((char *)config->device_name, locked_resource_name);
        locked_resource_name[sizeof(locked_resource_name) - 1] = '\0';
        snprintf(message,
                 sizeof(message),
                 "resource-lock: after UTSC_Rm_Lock status=%d resource=%s",
                 status,
                 locked_resource_name[0] != '\0' ? locked_resource_name : "unavailable");
        trace_stage(message);
        if (status != 0) {
            set_operation_error(result,
                                "resource-lock",
                                resource_error_message(runtime, "ATFS resource lock failed"),
                                runtime);
            return 0;
        }
        if (!copy_string(runtime->locked_resource_name,
                         sizeof(runtime->locked_resource_name),
                         locked_resource_name[0] != '\0' ? locked_resource_name : config->device_name)) {
            set_operation_error(result,
                                "resource-lock",
                                "ATFS locked resource name is too long",
                                runtime);
            runtime->resource_locked = 1;
            copy_string(runtime->locked_resource_name,
                        sizeof(runtime->locked_resource_name),
                        config->device_name);
            unlock_resource_if_needed(runtime);
            return 0;
        }
        runtime->resource_locked = 1;
    }

    snprintf(message, sizeof(message), "open-device: before UTSC_Gpib_Open(%s)", config->device_name);
    trace_stage(message);
    status = runtime->open((char *)config->device_name, &runtime->device_handle);
    snprintf(message,
             sizeof(message),
             "open-device: after UTSC_Gpib_Open status=%d (%s)",
             status,
             gpib_status_name(status));
    trace_stage(message);
    if (status != 0) {
        const char *library_message = runtime_error_message(runtime, "");
        snprintf(message,
                 sizeof(message),
                 "GPIB library could not open configured ATFS device (status=%d, name=%s)%s%s",
                 status,
                 gpib_status_name(status),
                 library_message[0] != '\0' ? ": " : "",
                 library_message);
        set_operation_error(result,
                            "open-device",
                            message,
                            runtime);
        unlock_resource_if_needed(runtime);
        return 0;
    }

    runtime->device_opened = 1;
    set_operation_success(result, "open-device", runtime);
    return 1;
}

int clear_device(gpib_runtime_t *runtime, operation_result_t *result) {
    int status = runtime->clear_device(runtime->device_handle);
    if (status != 0) {
        set_operation_error(result,
                            "clear-device",
                            runtime_error_message(runtime, "GPIB device clear failed"),
                            runtime);
        return 0;
    }
    set_operation_success(result, "clear-device", runtime);
    return 1;
}

int ensure_runtime_ready(gpib_runtime_t *runtime, const app_config_t *config, operation_result_t *result) {
    if (!load_library(runtime, config, result, LIBRARY_REQUIRE_GPIB)) {
        return 0;
    }
    if (!open_device(runtime, config, result)) {
        return 0;
    }
    return 1;
}

operation_result_t try_resource_lock(gpib_runtime_t *runtime, const app_config_t *config) {
    operation_result_t result;
    char locked_resource_name[MAX_DEVICE_NAME_LENGTH];
    const char *unlock_resource;
    char message[256];
    int status;
    int unlock_status;

    if (runtime->device_opened || runtime->resource_locked) {
        set_operation_error(&result,
                            "resource-trylock",
                            "Close the current GPIB connection before running RM trylock diagnostics",
                            runtime);
        return result;
    }

    if (!load_library(runtime, config, &result, LIBRARY_REQUIRE_RM_TRYLOCK)) {
        return result;
    }

    memset(locked_resource_name, 0, sizeof(locked_resource_name));
    snprintf(message,
             sizeof(message),
             "resource-trylock: before UTSC_Rm_TryLock(%s)",
             config->device_name);
    trace_stage(message);
    status = runtime->rm_trylock((char *)config->device_name, locked_resource_name);
    locked_resource_name[sizeof(locked_resource_name) - 1] = '\0';
    unlock_resource = locked_resource_name[0] != '\0' ? locked_resource_name : config->device_name;
    snprintf(message,
             sizeof(message),
             "resource-trylock: after UTSC_Rm_TryLock status=%d resource=%s",
             status,
             unlock_resource);
    trace_stage(message);

    if (status != 0) {
        snprintf(message,
                 sizeof(message),
                 "ATFS RM trylock failed (status=%d, resource=%s): %s",
                 status,
                 unlock_resource,
                 resource_error_message(runtime, "ATFS RM trylock failed"));
        set_operation_error(&result, "resource-trylock", message, runtime);
        return result;
    }

    if (!copy_string(runtime->locked_resource_name,
                     sizeof(runtime->locked_resource_name),
                     unlock_resource)) {
        snprintf(message,
                 sizeof(message),
                 "resource-trylock-unlock: before UTSC_Rm_Unlock(%s)",
                 config->device_name);
        trace_stage(message);
        unlock_status = runtime->rm_unlock((char *)config->device_name);
        snprintf(message,
                 sizeof(message),
                 "resource-trylock-unlock: after UTSC_Rm_Unlock status=%d",
                 unlock_status);
        trace_stage(message);
        set_operation_error(&result,
                            "resource-trylock",
                            "ATFS locked resource name is too long",
                            runtime);
        return result;
    }
    runtime->resource_locked = 1;
    copy_string(locked_resource_name,
                sizeof(locked_resource_name),
                runtime->locked_resource_name);

    snprintf(message,
             sizeof(message),
             "resource-trylock-unlock: before UTSC_Rm_Unlock(%s)",
             runtime->locked_resource_name);
    trace_stage(message);
    unlock_status = runtime->rm_unlock(runtime->locked_resource_name);
    snprintf(message,
             sizeof(message),
             "resource-trylock-unlock: after UTSC_Rm_Unlock status=%d",
             unlock_status);
    trace_stage(message);

    if (unlock_status != 0) {
        snprintf(message,
                 sizeof(message),
                 "ATFS RM trylock succeeded but unlock failed (status=%d, resource=%s): %s",
                 unlock_status,
                 runtime->locked_resource_name,
                 resource_error_message(runtime, "ATFS RM unlock failed"));
        set_operation_error(&result, "resource-trylock-unlock", message, runtime);
        return result;
    }

    runtime->resource_locked = 0;
    runtime->locked_resource_name[0] = '\0';
    set_operation_success(&result, "resource-trylock", runtime);
    snprintf(result.message,
             sizeof(result.message),
             "ATFS RM trylock succeeded and resource was unlocked (resource=%s)",
             locked_resource_name);
    return result;
}

operation_result_t probe_device(gpib_runtime_t *runtime, const app_config_t *config) {
    operation_result_t result;

    if (!ensure_runtime_ready(runtime, config, &result)) {
        return result;
    }
    set_operation_success(&result, "probe", runtime);
    return result;
}
