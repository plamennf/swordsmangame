#pragma once

#include "array.h"
#include "event.h"

Window_Type create_window(int width, int height, char *title);
void update_window_events();
void window_toggle_fullscreen(Window_Type hwnd);

void init_colors_and_utf8();
bool file_exists(char *fullpath);

char *get_path_of_running_executable();
void setcwd(char *path);

bool get_file_last_write_time(char *filepath, u64 *modtime);

double get_time();

void os_show_cursor(bool should_show);
void os_unconstrain_mouse();
void os_constrain_mouse(Window_Type window_handle);
void os_get_mouse_pointer_position(int *x, int *y, Window_Type window_handle, bool flipped);

bool os_directory_exists(char *dir);
bool os_make_directory_if_not_exist(char *dir);
