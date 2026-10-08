#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text_utils.h"

static void default_config(app_config_t *config) {
    memset(config, 0, sizeof(*config));
    copy_string(config->device_name, sizeof(config->device_name), DEFAULT_DEVICE_NAME);
    config->library_bind_mode = LIBRARY_BIND_NOW;
    config->read_buffer_size = DEFAULT_READ_BUFFER_SIZE;
    config->wait_srq_timeout_us = DEFAULT_WAIT_SRQ_TIMEOUT_US;
    config->use_rm_lock = 0;
}

const char *library_bind_mode_name(library_bind_mode_t bind_mode) {
    return bind_mode == LIBRARY_BIND_LAZY ? "lazy" : "now";
}

static int parse_library_bind_mode(const char *value, library_bind_mode_t *bind_mode) {
    if (strcmp(value, "now") == 0) {
        *bind_mode = LIBRARY_BIND_NOW;
        return 1;
    }
    if (strcmp(value, "lazy") == 0) {
        *bind_mode = LIBRARY_BIND_LAZY;
        return 1;
    }
    return 0;
}

static const char *non_empty_env(const char *name) {
    const char *value = getenv(name);
    return value != NULL && value[0] != '\0' ? value : NULL;
}

static int set_library_path(app_config_t *config, const char *path, char *error, size_t error_size) {
    if (!copy_string(config->library_path, sizeof(config->library_path), path)) {
        snprintf(error, error_size, "Config validation failed: resolved library_path is too long");
        return 0;
    }
    return 1;
}

static int set_library_path_from_parts(app_config_t *config,
                                       const char *base,
                                       const char *system_name,
                                       char *error,
                                       size_t error_size) {
    int written = snprintf(config->library_path,
                           sizeof(config->library_path),
                           "%s/%s/lib/libatfsgpib.so",
                           base,
                           system_name);
    if (written < 0 || (size_t)written >= sizeof(config->library_path)) {
        config->library_path[0] = '\0';
        snprintf(error, error_size, "Config validation failed: resolved library_path is too long");
        return 0;
    }
    return 1;
}

static int apply_default_library_path(app_config_t *config, char *error, size_t error_size) {
    const char *library_path;
    const char *atfs_system;
    const char *atfs_sys;
    const char *atfs_root;
    const char *atfs_arch;
    const char *atfs_os;
    char atfs_base[PATH_MAX];
    int written;

    if (config->library_path[0] != '\0') {
        return 1;
    }

    library_path = non_empty_env("GPIB_LIBRARY_PATH");
    if (library_path != NULL) {
        return set_library_path(config, library_path, error, error_size);
    }

    atfs_system = non_empty_env("ATFSSYSTEM");
    atfs_sys = non_empty_env("ATFSSYS");
    if (atfs_system != NULL && atfs_sys != NULL) {
        return set_library_path_from_parts(config, atfs_system, atfs_sys, error, error_size);
    }

    atfs_root = non_empty_env("ATFSROOT");
    atfs_arch = non_empty_env("ATFSARCH");
    atfs_os = non_empty_env("ATFSOS");
    if (atfs_root != NULL && atfs_arch != NULL && atfs_os != NULL && atfs_sys != NULL) {
        written = snprintf(atfs_base, sizeof(atfs_base), "%s/%s/%s", atfs_root, atfs_arch, atfs_os);
        if (written < 0 || (size_t)written >= sizeof(atfs_base)) {
            snprintf(error, error_size, "Config validation failed: resolved ATFS base path is too long");
            return 0;
        }
        return set_library_path_from_parts(config, atfs_base, atfs_sys, error, error_size);
    }

    return 1;
}

int load_config_file(const char *path, app_config_t *config, char *error, size_t error_size) {
    FILE *file = NULL;
    char line[MAX_LINE_LENGTH];
    int line_number = 0;

    default_config(config);

    file = fopen(path, "r");
    if (file == NULL) {
        snprintf(error, error_size, "Unable to open config file: %s", path);
        return 0;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char *separator;
        char *key;
        char *value;
        line_number++;

        trim_in_place(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        separator = strchr(line, '=');
        if (separator == NULL) {
            snprintf(error, error_size, "Invalid config line %d: missing '='", line_number);
            fclose(file);
            return 0;
        }

        *separator = '\0';
        key = line;
        value = separator + 1;
        trim_in_place(key);
        trim_in_place(value);

        if (strcmp(key, "library_path") == 0) {
            if (!copy_string(config->library_path, sizeof(config->library_path), value)) {
                snprintf(error, error_size, "Config line %d: library_path is too long", line_number);
                fclose(file);
                return 0;
            }
        } else if (strcmp(key, "preload_libraries") == 0) {
            if (!copy_string(config->preload_libraries, sizeof(config->preload_libraries), value)) {
                snprintf(error, error_size, "Config line %d: preload_libraries is too long", line_number);
                fclose(file);
                return 0;
            }
        } else if (strcmp(key, "library_bind_mode") == 0) {
            if (!parse_library_bind_mode(value, &config->library_bind_mode)) {
                snprintf(error, error_size, "Config line %d: library_bind_mode must be lazy or now", line_number);
                fclose(file);
                return 0;
            }
        } else if (strcmp(key, "device_name") == 0) {
            if (!copy_string(config->device_name, sizeof(config->device_name), value)) {
                snprintf(error, error_size, "Config line %d: device_name is too long", line_number);
                fclose(file);
                return 0;
            }
        } else if (strcmp(key, "board_index") == 0 ||
                   strcmp(key, "primary_address") == 0 ||
                   strcmp(key, "secondary_address") == 0 ||
                   strcmp(key, "timeout_ms") == 0) {
            continue;
        } else if (strcmp(key, "read_buffer_size") == 0) {
            if (!parse_int_value(value, &config->read_buffer_size)) {
                snprintf(error, error_size, "Config line %d: read_buffer_size must be an integer", line_number);
                fclose(file);
                return 0;
            }
        } else if (strcmp(key, "wait_srq_timeout_us") == 0) {
            if (!parse_int_value(value, &config->wait_srq_timeout_us)) {
                snprintf(error, error_size, "Config line %d: wait_srq_timeout_us must be an integer", line_number);
                fclose(file);
                return 0;
            }
        } else if (strcmp(key, "use_rm_lock") == 0) {
            if (!parse_bool_value(value, &config->use_rm_lock)) {
                snprintf(error, error_size, "Config line %d: use_rm_lock must be 0/1, false/true, no/yes, or off/on", line_number);
                fclose(file);
                return 0;
            }
        } else {
            snprintf(error, error_size, "Config line %d: unsupported key '%s'", line_number, key);
            fclose(file);
            return 0;
        }
    }

    fclose(file);

    if (!apply_default_library_path(config, error, error_size)) {
        return 0;
    }

    if (config->library_path[0] == '\0') {
        snprintf(error, error_size, "Config validation failed: library_path is required");
        return 0;
    }
    if (config->device_name[0] == '\0') {
        snprintf(error, error_size, "Config validation failed: device_name is required");
        return 0;
    }
    if (config->read_buffer_size <= 0 || config->read_buffer_size > MAX_PAYLOAD_LENGTH) {
        snprintf(error, error_size, "Config validation failed: read_buffer_size must be between 1 and %d", MAX_PAYLOAD_LENGTH);
        return 0;
    }
    if (config->wait_srq_timeout_us <= 0) {
        snprintf(error, error_size, "Config validation failed: wait_srq_timeout_us must be greater than 0");
        return 0;
    }

    return 1;
}
