#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#include <stddef.h>

void trim_in_place(char *text);
int copy_string(char *destination, size_t destination_size, const char *source);
int parse_int_value(const char *text, int *value);
int parse_bool_value(const char *text, int *value);
int is_ascii_text(const char *text);

#endif
