#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

#include "gpib_test.h"

typedef enum {
    LIBRARY_BIND_NOW = 0,
    LIBRARY_BIND_LAZY = 1
} library_bind_mode_t;

typedef struct {
    char library_path[PATH_MAX];
    char preload_libraries[MAX_LINE_LENGTH];
    library_bind_mode_t library_bind_mode;
    char device_name[MAX_DEVICE_NAME_LENGTH];
    int read_buffer_size;
    int wait_srq_timeout_us;
    int use_rm_lock;
} app_config_t;

int load_config_file(const char *path, app_config_t *config, char *error, size_t error_size);
const char *library_bind_mode_name(library_bind_mode_t bind_mode);

#endif
