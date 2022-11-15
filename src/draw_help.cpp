#include "pch.h"
#include "draw_help.h"
#include "render.h"
#include "font.h"

void rendering_2d_right_handed(int width, int height) {
    Matrix4 m;
    m.identity();

    float w = (float)width;
    if (w < 1.0f) w = 1.0f;
    float h = (float)height;
    if (h < 1.0f) h = 1.0f;
    
    m._11 = 2.0f / w;
    m._22 = 2.0f / h;
    m._14 = -1.0f;
    m._24 = -1.0f;
    
    global_parameters.proj_matrix = m;
    global_parameters.view_matrix.identity();
    global_parameters.transform = global_parameters.proj_matrix * global_parameters.view_matrix;    
}

void draw_text(Dynamic_Font *font, char *text, int x, int y, Vector4 color) {
    Texture *last_texture = NULL;
    
    font->prep_text(text, x, y);
    immediate_begin();
    for (Font_Quad quad : font->font_quads) {
        Vector2 p0(quad.x0, quad.y0);
        Vector2 p1(quad.x1, quad.y0);
        Vector2 p2(quad.x1, quad.y1);
        Vector2 p3(quad.x0, quad.y1);

        Vector2 uv0(quad.u0, quad.v1);
        Vector2 uv1(quad.u1, quad.v1);
        Vector2 uv2(quad.u1, quad.v0);
        Vector2 uv3(quad.u0, quad.v0);

        if (last_texture != quad.texture) {
            set_texture(0, quad.texture);
            last_texture = quad.texture;
        }

        immediate_quad(p0, p1, p2, p3, uv0, uv1, uv2, uv3, color);
    }
    immediate_flush();

    font->font_quads.count = 0;
}
