#pragma once

enum Cursor_Type {
    CURSOR_TYPE_DOT,
    CURSOR_TYPE_FOUR_ARROWS,
};

void draw_cursor(bool center_cursor, Cursor_Type cursor_type);
