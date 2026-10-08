#include "text_utils.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

void trim_in_place(char *text) {
    size_t length;
    char *start;

    if (text == NULL) {
        return;
    }

    start = text;
    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }

    if (start != text) {
        memmove(text, start, strlen(start) + 1);
    }

    length = strlen(text);
    while (length > 0 && isspace((unsigned char)text[length - 1])) {
        text[length - 1] = '\0';
        length--;
    }
}

int copy_string(char *destination, size_t destination_size, const char *source) {
    if (destination == NULL || destination_size == 0 || source == NULL) {
        return 0;
    }
    if (strlen(source) >= destination_size) {
        return 0;
    }
    strcpy(destination, source);
    return 1;
}

int parse_int_value(const char *text, int *value) {
    char *end = NULL;
    long parsed;

    if (text == NULL || *text == '\0' || value == NULL) {
        return 0;
    }

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX) {
        return 0;
    }

    *value = (int)parsed;
    return 1;
}

int parse_bool_value(const char *text, int *value) {
    if (text == NULL || value == NULL) {
        return 0;
    }
    if (strcmp(text, "1") == 0 || strcmp(text, "true") == 0 ||
        strcmp(text, "yes") == 0 || strcmp(text, "on") == 0) {
        *value = 1;
        return 1;
    }
    if (strcmp(text, "0") == 0 || strcmp(text, "false") == 0 ||
        strcmp(text, "no") == 0 || strcmp(text, "off") == 0) {
        *value = 0;
        return 1;
    }
    return 0;
}

int is_ascii_text(const char *text) {
    const unsigned char *current = (const unsigned char *)text;

    if (text == NULL) {
        return 0;
    }

    while (*current != '\0') {
        if (*current > 0x7f) {
            return 0;
        }
        current++;
    }

    return 1;
}
