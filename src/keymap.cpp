#include "pch.h"
#include "keymap.h"
#include "text_file_handler.h"
#include "event.h"
#include "array.h"
#include "os.h"

#include <ctype.h>

static Key_Code parse_key_code(char *string) {
    if (string_length(string) == 1) {
        string[0] = toupper(string[0]);
        if ((string[0] >= 48 && string[0] <= 57) || // If it is a number
            (string[0] >= 65 && string[0] <= 90)) { // If it is a letter
            return (Key_Code)string[0];
        }
    }

    char *s = lowercase(string);

    if (strings_match(s, "backspace")) return KEY_BACKSPACE;
    else if (strings_match(s, "tab")) return KEY_TAB;
    else if (strings_match(s, "escape")) return KEY_ESCAPE;
    else if (strings_match(s, "space")) return KEY_SPACE;
    
    else if (strings_match(s, "f1")) return KEY_F1;
    else if (strings_match(s, "f2")) return KEY_F2;
    else if (strings_match(s, "f3")) return KEY_F3;
    else if (strings_match(s, "f4")) return KEY_F4;
    else if (strings_match(s, "f5")) return KEY_F5;
    else if (strings_match(s, "f6")) return KEY_F6;
    else if (strings_match(s, "f7")) return KEY_F7;
    else if (strings_match(s, "f8")) return KEY_F8;
    else if (strings_match(s, "f9")) return KEY_F9;
    else if (strings_match(s, "f10")) return KEY_F10;
    else if (strings_match(s, "f11")) return KEY_F11;
    else if (strings_match(s, "f12")) return KEY_F12;

    else if (strings_match(s, "enter")) return KEY_ENTER;
    
    else if (strings_match(s, "shift")) return KEY_SHIFT;
    else if (strings_match(s, "ctrl")) return KEY_CTRL;
    else if (strings_match(s, "alt")) return KEY_ALT;

    else if (strings_match(s, "mousebuttonleft")) return MOUSE_BUTTON_LEFT;
    else if (strings_match(s, "mousebuttonright")) return MOUSE_BUTTON_RIGHT;

    else if (strings_match(s, "up")) return KEY_UP;
    else if (strings_match(s, "down")) return KEY_DOWN;
    else if (strings_match(s, "right")) return KEY_RIGHT;
    else if (strings_match(s, "left")) return KEY_LEFT;

    else {
        log_error("Unknown key code: %s\n", s);
    }
    
    return KEY_UNKNOWN;
}

static void parse_key_action(char *line, Key_Action *action) {
    char *at = line;

    action->key_code = KEY_UNKNOWN;
    action->alt_down = false;
    action->shift_down = false;
    action->ctrl_down = false;
    
    while (true) {
        Array <char> key_string;
        key_string.use_temporary_storage = true;

        bool reached_end_of_line = false;
        while (*at != '-') {
            if (!*at) {
                reached_end_of_line = true;
                break;
            }

            key_string.add(at[0]);
            at++;
        }
        key_string.add(0);
        
        if (*at == '-') {
            at++;
        }

        // The format for a key action is Mod-Mod-Mod-KeyCode
        // For example Ctrl-Alt-Shift-G
        // The KeyCode is always at the end
        // so we only check if it is a key code if it is at the end of the line
        // otherwise we check if it is a modifier
        if (reached_end_of_line) {
            action->key_code = parse_key_code(key_string.data);
        } else {
            if (strings_match(lowercase(key_string.data), "ctrl")) {
                action->ctrl_down = true;
            } else if (strings_match(lowercase(key_string.data), "alt")) {
                action->alt_down = true;
            } else if (strings_match(lowercase(key_string.data), "shift")) {
                action->shift_down = true;
            } else {
                log_error("Expected a modifier but found: %s\n", key_string.data);
                log_error("Valid modifiers are:\n");
                log_error("    Alt\n");
                log_error("    Shift\n");
                log_error("    Ctrl\n");
                return;
            }
        }
        
        if (reached_end_of_line) break;
    }
}

bool load_keymap(Keymap *keymap, char *filepath) {
    Text_File_Handler handler;
    handler.start_file(filepath, filepath, "load_keymap");
    if (handler.failed) return false;

    set_keys_to_default(keymap);
    
    while (true) {
        char *line = handler.consume_next_line();
        if (!line) break;

        if (starts_with(line, "MoveLeft")) {
            line += 8;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);
            
            parse_key_action(line, &keymap->move_left);
        } else if (starts_with(line, "MoveRight")) {
            line += 9;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);
            
            parse_key_action(line, &keymap->move_right);
        } else if (starts_with(line, "MoveUp")) {
            line += 6;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);
            
            parse_key_action(line, &keymap->move_up);
        } else if (starts_with(line, "MoveDown")) {
            line += 8;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);
            
            parse_key_action(line, &keymap->move_down);
        } else if (starts_with(line, "SaveCurrentGameMode")) {
            line += 19;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);
            
            parse_key_action(line, &keymap->save_current_game_mode);
        } else if (starts_with(line, "ToggleFullscreen")) {
            line += 16;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);
            
            parse_key_action(line, &keymap->toggle_fullscreen);
        }
    }

    get_file_last_write_time(filepath, &keymap->modtime);
    
    return true;
}

void set_keys_to_default(Keymap *keymap) {
    keymap->move_left.key_code = 'A';
    keymap->move_right.key_code = 'D';
    keymap->move_up.key_code = 'W';
    keymap->move_down.key_code = 'S';

    keymap->save_current_game_mode.key_code = 'P';
    keymap->save_current_game_mode.ctrl_down = true;
    keymap->toggle_fullscreen.key_code = KEY_F11;
}
