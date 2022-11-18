#pragma once

#define KEYMAP_FILE_VERSION 1

struct Key_Action {
    int key_code;
    bool alt_down;
    bool shift_down;
    bool ctrl_down;
};

struct Keymap {
    u64 modtime = 0;
    
    Key_Action move_left = {};
    Key_Action move_right = {};
    Key_Action move_up = {};
    Key_Action move_down = {};

    Key_Action save_current_game_mode = {};
    Key_Action toggle_fullscreen = {};
    Key_Action toggle_editor = {};
};

bool load_keymap(Keymap *keymap, char *filepath);
void set_keys_to_default(Keymap *keymap);

void keymap_do_hotloading();
