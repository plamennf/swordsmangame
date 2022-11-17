#include "pch.h"
#include "cursor.h"
#include "render.h"
#include "draw_help.h"
#include "os.h"
#include "game.h"

Vector2 get_vec2(float theta) {
    float ct = cosf(theta);
    float st = sinf(theta);

    return Vector2(ct, st);
}

void draw_cursor() {
    int x, y;
    os_get_mouse_pointer_position(&x, &y, globals.my_window, true);

    Vector2 center;
    center.x = static_cast <float>(x);
    center.y = static_cast <float>(y);
    
    immediate_set_shader(globals.shader_color);
    rendering_2d_right_handed(globals.render_width, globals.render_height);
    refresh_global_parameters();
    
    float r = 0.01f * globals.render_height;
    
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
