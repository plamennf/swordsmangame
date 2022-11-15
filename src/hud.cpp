#include "pch.h"
#include "hud.h"
#include "game.h"
#include "render.h"
#include "font.h"
#include "draw_help.h"

const double NUM_SECONDS_BETWEEN_UPDATES = 0.05;
static double num_seconds_since_last_update;
static int num_frames_since_last_update;
static double accumulated_dt;
static double dt_for_draw;

static void draw_fps() {
    immediate_set_shader(globals.shader_text);

    float dt = globals.time_info.current_dt;

    num_seconds_since_last_update += dt;
    num_frames_since_last_update++;
    if (num_seconds_since_last_update >= NUM_SECONDS_BETWEEN_UPDATES) {
        num_seconds_since_last_update = 0;

        dt_for_draw = accumulated_dt / num_frames_since_last_update;

        accumulated_dt = 0.0;
        num_frames_since_last_update = 0;
    }
    accumulated_dt += dt;
    
    int font_size = (int)(0.02f * globals.render_height);
    Dynamic_Font *font = get_font_at_size("OpenSans-SemiBold", font_size);

    if (!dt_for_draw) dt_for_draw = 1.0;
    char *text = tprint("%.2lf fps", 1.0 / dt_for_draw);
    
    int x = globals.render_width - font->get_string_width_in_pixels(text);
    int y = globals.render_height - font->character_height;

    int offset = font->character_height / 20;
    
    draw_text(font, text, x+offset, y-offset, Vector4(0, 0, 0, 1));
    draw_text(font, text, x, y, Vector4(1, 1, 1, 1));
}

void draw_hud() {
    rendering_2d_right_handed(globals.render_width, globals.render_height);
    refresh_global_parameters();

#ifdef _DEBUG
    draw_fps();
#endif
}
