#include "pch.h"
#include "general.h"
#include "array.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct Temporary_Storage {
    s64 size = 0;
    s64 occupied = 0;
    u8 *data = 0;
};

static Temporary_Storage temporary_storage;

void init_temporary_storage(s64 size) {
    temporary_storage.size = size;
    temporary_storage.occupied = 0;
    temporary_storage.data = (u8 *)malloc(size);
    memset(temporary_storage.data, 0, size);
}

void reset_temporary_storage() {
    temporary_storage.occupied = 0;
}

s64 get_temporary_storage_mark() {
    return temporary_storage.occupied;
}

void set_temporary_storage_mark(s64 mark) {
    temporary_storage.occupied = mark;
}

void *talloc(s64 size, s64 alignment) {
    while (temporary_storage.occupied + size > temporary_storage.size) {
        u8 *new_data = (u8 *)malloc(temporary_storage.size * 2);
        memcpy(new_data, temporary_storage.data, temporary_storage.size);
        memset(new_data + size, 0, temporary_storage.size);
        free(temporary_storage.data);
        temporary_storage.data = new_data;
        temporary_storage.size *= 2;
    }

    s64 alignment_offset = 0;

    s64 result_pointer = (s64)temporary_storage.data + temporary_storage.occupied;
    s64 alignment_mask = alignment - 1;
    if (result_pointer & alignment_mask) {
        alignment_offset = alignment - (result_pointer & alignment_mask);
    }
    
    void *result = (void *)(temporary_storage.data + temporary_storage.occupied);
    temporary_storage.occupied += size + alignment_offset;
    return result;
}

char *sprint(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    size_t n = 1 + vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    char *str = (char *)malloc(n);
    va_start(args, fmt);
    vsnprintf(str, n, fmt, args);
    va_end(args);
    return str;
}

char *sprint_valist(char *fmt, va_list args) {
    va_list ap = args;
    size_t n = 1 + vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    char *str = (char *)malloc(n);
    vsnprintf(str, n, fmt, args);
    va_end(args);
    return str;
}

char *tprint(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    size_t n = 1 + vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    char *str = (char *)talloc(n);
    va_start(args, fmt);
    vsnprintf(str, n, fmt, args);
    va_end(args);
    return str;
}

char *tprint_valist(char *fmt, va_list args) {
    va_list ap = args;
    size_t n = 1 + vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    char *str = (char *)talloc(n);
    vsnprintf(str, n, fmt, args);
    va_end(args);
    return str;
}

char *copy_string(char *s, bool use_temporary_storage) {
    auto len = string_length(s);
    char *result;
    if (use_temporary_storage) {
        result = static_cast <char *>(talloc(len + 1));
    } else {
        result = new char[len + 1];
    }
    memcpy(result, s, len + 1);
    return result;
}

bool strings_match(char *a, char *b) {
    if (!a || !b) return false;
    return strcmp(a, b) == 0;
}

// Copy-paste from https://github.com/raysan5/raylib/blob/master/src/rtext.c
int get_codepoint(char *text, int *bytes_processed) {
    int code = 0x3f;
    int octet = (u8)(text[0]);
    *bytes_processed = 1;

    if (octet <= 0x7f) {
        // Only one octet (ASCII range x00-7F)
        code = text[0];
    } else if ((octet & 0xe0) == 0xc0) {
        // Two octets

        // [0]xC2-DF     [1]UTF8-tail(x80-BF)
        u8 octet1 = text[1];
        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytes_processed = 2; return code; } // Unexpected sequence

        if ((octet >= 0xc2) && (octet <= 0xdf)) {
            code = ((octet & 0x1f) << 6) | (octet1 & 0x3f);
            *bytes_processed = 2;
        }
    } else if ((octet & 0xf0) == 0xe0) {
        u8 octet1 = text[1];
        u8 octet2 = '\0';

        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytes_processed = 2; return code; } // Unexpected sequence

        octet2 = text[2];

        if ((octet2 == '\0') || ((octet2 >> 6) != 2)) { *bytes_processed = 3; return code; } // Unexpected sequence

        // [0]xE0    [1]xA0-BF       [2]UTF8-tail(x80-BF)
        // [0]xE1-EC [1]UTF8-tail    [2]UTF8-tail(x80-BF)
        // [0]xED    [1]x80-9F       [2]UTF8-tail(x80-BF)
        // [0]xEE-EF [1]UTF8-tail    [2]UTF8-tail(x80-BF)

        if (((octet == 0xe0) && !((octet1 >= 0xa0) && (octet1 <= 0xbf))) ||
            ((octet == 0xed) && !((octet1 >= 0x80) && (octet1 <= 0x9f)))) {
            *bytes_processed = 2;
            return code;
        }

        if ((octet >= 0xe0) && (0 <= 0xef)) {
            code = ((octet & 0xf) << 12) | ((octet1 & 0x3f) << 6) | (octet2 & 0x3f);
            *bytes_processed = 3;
        }
    } else if ((octet & 0xf8) == 0xf0) {
        // Four octets
        if (octet > 0xf4) return code;

        u8 octet1 = text[1];
        u8 octet2 = '\0';
        u8 octet3 = '\0';

        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytes_processed = 2; return code; } // Unexpected sequence

        octet2 = text[2];

        if ((octet2 == '\0') || ((octet2 >> 6) != 2)) { *bytes_processed = 3; return code; } // Unexpected sequence

        octet3 = text[3];

        if ((octet3 == '\0') || ((octet3 >> 6) != 2)) { *bytes_processed = 4; return code; }  // Unexpected sequence

        // [0]xF0       [1]x90-BF       [2]UTF8-tail  [3]UTF8-tail
        // [0]xF1-F3    [1]UTF8-tail    [2]UTF8-tail  [3]UTF8-tail
        // [0]xF4       [1]x80-8F       [2]UTF8-tail  [3]UTF8-tail

        if (((octet == 0xf0) && !((octet1 >= 0x90) && (octet1 <= 0xbf))) ||
            ((octet == 0xf4) && !((octet1 >= 0x80) && (octet1 <= 0x8f)))) {
            *bytes_processed = 2; return code; // Unexpected sequence
        }

        if (octet >= 0xf0) {
            code = ((octet & 0x7) << 18) | ((octet1 & 0x3f) << 12) | ((octet2 & 0x3f) << 6) | (octet3 & 0x3f);
            *bytes_processed = 4;
        }
    }

    if (code > 0x10ffff) code = 0x3f; // Codepoints after U+10ffff are invalid

    return code;
}

char *eat_spaces(char *s) {
    if (!s) return NULL;

    while (isspace(*s)) {
        s++;
    }
    return s;
}

bool starts_with(char *string, char *substring) {
    if (!string || !substring) return false;

    size_t len = string_length(substring);
    for (size_t i = 0; i < len; i++) {
        if (string[i] != substring[i]) {
            return false;
        }
    }

    return true;
}

char *consume_next_line(char **text_ptr) {
    char *t = *text_ptr;
    if (!*t) return NULL;

    char *s = t;

    while (*t && (*t != '\n') && (*t != '\r')) t++;

    char *end = t;
    if (*t) {
        end++;

        if (*t == '\r') {
            if (*end == '\n') ++end;
        }

        *t = '\0';
    }
    
    *text_ptr = end;
    
    return s;
}

float fract(float value) {
    int intvalue = (int)value;
    float fractpart = value - intvalue;
    return fractpart;
}

char *concatenate_with_commas(char **array, s64 array_count, bool use_temporary_storage) {
    Array <char> result;
    result.use_temporary_storage = true;

    {
        s64 to_reserve = 1; // For zero-termination
        for (s64 i = 0; i < array_count; i++) {
            char *s = array[i];
            to_reserve += string_length(s);
            if (i != array_count-1) to_reserve += 2; // For a comma and a space
        }
        result.reserve((int)to_reserve);
    }

    for (s64 i = 0; i < array_count; i++) {
        char *s = array[i];
        for (char *at = s; *at; at++) {
            result.add(*at);
        }
        if (i != array_count-1) {
            result.add(',');
            result.add(' ');
        }
    }
    result.add(0);

    return result.data;
}

char *concatenate(char *a, char *b) {
    s64 a_len = string_length(a);
    s64 b_len = string_length(b);
    s64 len = a_len + b_len;
    char *result = new char[len + 1];
    result[len] = 0;
    memcpy(result, a, a_len);
    memcpy(result + a_len, b, b_len);
    return result;
}

s64 string_length(char *s) {
    if (!s) return 0;

    s64 length = 0;
    while (*s++) {
        length++;
    }
    return length;
}

void log(char *string, ...) {
    s64 mark = get_temporary_storage_mark();
    defer { set_temporary_storage_mark(mark); };
    
    va_list args;
    va_start(args, string);
    char *buf = tprint_valist(string, args);
    va_end(args);

    printf("%s", buf); // Replace with WriteFile
}

void log_error(char *string, ...) {
    s64 mark = get_temporary_storage_mark();
    defer { set_temporary_storage_mark(mark); };
    
    va_list args;
    va_start(args, string);
    char *buf = tprint_valist(string, args);
    va_end(args);

    char *prefix = "\033[1;31m";
    char *postfix = "\033[0m";
    printf("%s%s%s", prefix, buf, postfix); // Replace with WriteFile
}

void print(char *string, ...) {
    s64 mark = get_temporary_storage_mark();
    defer { set_temporary_storage_mark(mark); };
    
    va_list args;
    va_start(args, string);
    char *buf = tprint_valist(string, args);
    va_end(args);

    printf("%s", buf); // Replace with WriteFile
}

char *find_character_from_left(char *string, char c) {
    if (!string) return NULL;
    return strchr(string, c);
}

char *eat_trailing_spaces(char *string) {
    if (!string) return NULL;

    char *end = string + string_length(string) - 1;
    while (end > string && isspace(*end)) end--;

    end[1] = 0;

    return string;
}

bool contains(char *string, char *substring) {
    if (!string || !substring) return false;
    return strstr(string, substring);
}

char *replace_backslash_with_forwardslash(char *string) {
    if (!string) return NULL;

    for (char *at = string; *at; at++) {
        if (*at == '\\') {
            *at = '/';
        }
    }

    return string;
}

char *replace_forwardslash_with_backslash(char *string) {
    if (!string) return NULL;

    for (char *at = string; *at; at++) {
        if (*at == '/') {
            *at = '\\';
        }
    }
    
    return string;    
}

char *lowercase(char *string) {
    if (!string) return NULL;

    for (char *at = string; *at; at++) {
        *at = tolower(*at);
    }
    return string;
}

char *copy_strip_extension(char *filename, bool use_temporary_storage) {
    if (!filename) return NULL;

    char *result = copy_string(filename, use_temporary_storage);
    char *dot = strrchr(result, '.');
    result[dot - result] = 0;
    return result;
}

char *concatenate_with_newlines(char **array, s64 array_count, bool use_temporary_storage) {
    s64 dest_string_length = 1;
    for (s64 i = 0; i < array_count; i++) {
        dest_string_length += string_length(array[i]);
        dest_string_length += 1;
    }
    
    char *dest;
    if (use_temporary_storage) {
        dest = static_cast <char *>(talloc(dest_string_length));
    } else {
        dest = new char[dest_string_length];
    }
    s64 offset = 0;
    for (s64 i = 0; i < array_count; i++) {
        char *s = array[i];
        s64 len = string_length(s);
        memcpy(dest + offset, s, len);
        dest[offset + len] = '\n';
        offset += len + 1;
    }
    dest[dest_string_length-1] = 0;

    return dest;
}

void Memory_Arena::init(s64 _size) {
    occupied = 0;
    size = _size;
    data = (u8 *)malloc(size);
}

void *Memory_Arena::get(s64 _size) {
    if (occupied + _size > size) {
        s64 old_size = size;
        s64 new_size = Max(size * 2, size + _size);

        u8 *new_data = (u8 *)malloc(new_size);
        if (data) {
            memcpy(new_data, data, old_size);
            memset(new_data + old_size, 0, new_size - old_size);
            free(data);
        } else {
            memset(new_data, 0, new_size);
        }

        data = new_data;
        size = new_size;
    }

    void *result = (void *)(data + occupied);
    occupied += _size;
    return result;
}
