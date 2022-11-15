#pragma once

#include "hash_table.h"
#include "array.h"

struct Texture;

struct Loaded_Font {
    char *name;
    struct FT_FaceRec_ *face;
};

struct Font_Page {
    int cursor_x;
    int cursor_y;
    Texture *texture;
};

struct Glyph_Data {
    int x0, y0;
    int width, height;
    int offset_x, offset_y;
    int advance;
    Texture *texture;
};

struct Font_Quad {
    float x0, y0;
    float x1, y1;
    float u0, v0;
    float u1, v1;
    Texture *texture;
};

struct Dynamic_Font {
    char *name = NULL;

    struct FT_FaceRec_ *face = NULL;
    Hash_Table <int, Glyph_Data *> glyph_lookup;
    int character_height = 0;
    
    Array <Font_Page *> font_pages;
    Font_Page *current_page = NULL;

    Array <Font_Quad> font_quads;
    
    void load(Loaded_Font *font, int size);
    Glyph_Data *get_or_load_glyph(int utf32);
    int get_string_width_in_pixels(char *text);

    void prep_text(char *text, int x, int y);
    
private:
    void advance_current_page(Glyph_Data *data, int *old_x, int *old_y);
    void generate_font_quads(char *text, int x, int y);
};

Dynamic_Font *get_font_at_size(char *name, int size);
