#pragma once

enum Key_Code {
    KEY_UNKNOWN = 0,

    KEY_BACKSPACE = 8,
    KEY_TAB = 9,
    KEY_ESCAPE = 27,
    KEY_SPACE = 32,

    KEY_F1 = 129,
    KEY_F2 = 130,
    KEY_F3 = 131,
    KEY_F4 = 132,
    KEY_F5 = 133,
    KEY_F6 = 134,
    KEY_F7 = 135,
    KEY_F8 = 136,
    KEY_F9 = 137,
    KEY_F10 = 138,
    KEY_F11 = 139,
    KEY_F12 = 140,

    KEY_ENTER = 153,

    KEY_SHIFT = 189,
    KEY_CTRL = 190,
    KEY_ALT = 191,

    MOUSE_BUTTON_LEFT = 210,
    MOUSE_BUTTON_RIGHT = 211,

    KEY_UP = 267,
    KEY_DOWN = 268,
    KEY_RIGHT = 269,
    KEY_LEFT = 270,
    
    NUM_KEY_CODES,
};

enum Event_Type {
    EVENT_TYPE_UNKNOWN,
    EVENT_TYPE_QUIT,
    EVENT_TYPE_KEYBOARD,
    EVENT_TYPE_MOUSE_MOVE,
};

struct Event {
    Event_Type type = EVENT_TYPE_UNKNOWN;

    Key_Code key_code = KEY_UNKNOWN;
    bool key_pressed = false;
    bool alt_pressed = false;

    int x = 0;
    int y = 0;
};

struct Window_Resize_Record {
    int width;
    int height;
    Window_Type window;
};
