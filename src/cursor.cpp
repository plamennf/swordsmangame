#include "pch.h"
#include "cursor.h"
#include "render.h"
#include "draw_help.h"
#include "os.h"
#include "game.h"

#include "texture_registry.h"

static void draw_cursor_dot(int x, int y) {
    Vector2 center;
    center.x = static_cast <float>(x);
    center.y = static_cast <float>(y);
    
    float r = 0.01f * globals.render_height;

    immediate_set_shader(globals.shader_color);
    
    immediate_begin();
    
    const int NUM_TRIANGLES = 100;
    auto dtheta = TAU / NUM_TRIANGLES;
    
    auto color = Vector4(1, 1, 1, 1);
    
    for (int i = 0; i < NUM_TRIANGLES; i++) {
        auto theta0 = i * dtheta;
        auto theta1 = (i+1) * dtheta;
        
        auto v0 = get_vec2(theta0);
        auto v1 = get_vec2(theta1);

        auto p0 = center;
        auto p1 = center + r * v0;
        auto p2 = center + r * v1;

        immediate_triangle(p0, p1, p2, color);
    }
    
    immediate_flush();
}

static void draw_cursor_four_arrows(int x, int y) {
    Vector2 center;
    center.x = static_cast <float>(x);
    center.y = static_cast <float>(y);

    immediate_set_shader(globals.shader_texture);
    
    Texture *texture = globals.texture_registry->get("four-arrow");
    if (!texture) return;
    set_texture(0, texture);

    float scale = 0.025f * globals.display_height;
        
    Vector2 p0 = center + Vector2(-scale, -scale);
    Vector2 p1 = center + Vector2(+scale, -scale);
    Vector2 p2 = center + Vector2(+scale, +scale);
    Vector2 p3 = center + Vector2(-scale, +scale);

    Vector2 uv0(0, 0);
    Vector2 uv1(1, 0);
    Vector2 uv2(1, 1);
    Vector2 uv3(0, 1);
    
    immediate_begin();
    immediate_quad(p0, p1, p2, p3, uv0, uv1, uv2, uv3, Vector4(1, 1, 1, 1));
    immediate_flush();
}

void draw_cursor(bool center_cursor, Cursor_Type cursor_type) {
    int x, y;
    if (center_cursor) {
        x = globals.display_width / 2;
        y = globals.display_height / 2;
    } else {
        os_get_mouse_pointer_position(&x, &y, globals.my_window, true);
    }
    
    rendering_2d_right_handed(globals.display_width, globals.display_height);
    refresh_global_parameters();
    
    if (cursor_type == CURSOR_TYPE_DOT) {
        draw_cursor_dot(x, y);
    } else if (cursor_type == CURSOR_TYPE_FOUR_ARROWS) {
        draw_cursor_four_arrows(x, y);
    }
}
