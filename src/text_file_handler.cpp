#include "pch.h"
#include "text_file_handler.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

Text_File_Handler::~Text_File_Handler() {
    delete [] orig_file_data;
}

void Text_File_Handler::start_file(char *_short_name, char *_full_path, char *_log_agent) {
    short_name = _short_name;
    full_path = _full_path;
    log_agent = _log_agent;

    bool success = false;
    char *read_entire_text_file(char *filepath);
    file_data = read_entire_text_file(full_path);
    orig_file_data = file_data;

    if (!file_data) {
        log_error("[%s] Unable to load file '%s'.\n", log_agent, full_path);
        failed = true;
        return;
    }

    if (do_version_number) {
        char *line = consume_next_line();
        assert(line);
        
        if (!line) {
            log_error("[%s] Unable to find a version number at the top of file '%s'!\n", log_agent, full_path);
            failed = true;
            return;
        }

        assert(line);
        if (line[0] != '[') {
            log_error("[%s] Expected '[' at the top of file '%s', but did not get it!\n", log_agent, full_path);
            failed = true;
            return;
        }

        line += 1;

        version = atoi(line);
    }
}

char *Text_File_Handler::consume_next_line() {
    while (true) {
        char *line = ::consume_next_line(&file_data);
        if (!line) return nullptr;
        
        line_number += 1;

        line = eat_spaces(line);

        if (!line || strings_match(line, "")) continue;

        if (strip_comments_from_end_of_lines) {
            char *rhs = find_character_from_left(line, comment_character);
            if (rhs) {
                char *orig_line = line;
                line[string_length(orig_line) - string_length(rhs)] = 0;
                if (string_length(line) == 0) continue;
            }
        } else {
            if (line[0] == comment_character) continue;
        }
        
        line = eat_trailing_spaces(line);
        assert(string_length(line) > 0);

        return line;
    }
}

void Text_File_Handler::report_error(char *fmt, ...) {
    s64 mark = get_temporary_storage_mark();
    defer { set_temporary_storage_mark(mark); };
    
    va_list args;
    va_start(args, fmt);
    char *buf = tprint_valist(fmt, args);
    va_end(args);

    log_error("Error in line %d of file '%s': %s\n", line_number, full_path, buf);
}
