#pragma once

struct Text_File_Handler {
    char *short_name = 0;
    char *full_path = 0;

    char *log_agent = 0;

    char comment_character = '#';

    bool do_version_number = true;
    bool strip_comments_from_end_of_lines = true;

    char *file_data = 0;
    char *orig_file_data = 0;

    bool failed = false;
    int version = -1;

    int line_number = 0;

    ~Text_File_Handler();

    void start_file(char *short_name, char *full_path, char *log_agent);
    char *consume_next_line();
    void report_error(char *fmt, ...);
};
