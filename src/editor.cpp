#include "pch.h"
#include "editor.h"
#include "render.h"
#include "draw_help.h"
#include "game.h"
#include "os.h"

static Vector2i currently_selected_tile;
static bool has_currently_selected_tile;

void update_editor() {
    if (is_key_down(MOUSE_BUTTON_LEFT)) {
        int x, y;
        os_get_mouse_pointer_position(&x, &y, globals.my_window, true);

        float w = (float)globals.display_width;
        if (w < 1.0f) w = 1.0f;
        float h = (float)globals.display_height;
        if (h < 1.0f) h = 1.0f;

        x -= globals.render_area.x;
        y -= globals.render_area.y;
        
        float nx = (float)x / w;
        float ny = (float)y / h;
        
        nx *= globals.world_space_size_x;
        ny *= globals.world_space_size_y;
        
        int ix = (int)nx;
        int iy = (int)ny;

        currently_selected_tile = Vector2i(ix, iy);
        has_currently_selected_tile = true;
    }
}

static void draw_outline(Vector2i _position) {
    Vector2 position((float)_position.x, (float)_position.y);
    position.x -= 8.0f;
    position.y -= 4.5f;

    Vector4 color(0, 1, 0, 1);
    
    float horizontal_line_width = 1.0f;
    float horizontal_line_height = 0.01f;
    float vertical_line_width = 0.01f;
    float vertical_line_height = 1.0f - 2.0f*horizontal_line_height;
    
    immediate_begin();

    {
        Vector2 p0(position.x, position.y);
        Vector2 p1(position.x + horizontal_line_width, position.y);
        Vector2 p2(position.x + horizontal_line_width, position.y + horizontal_line_height);
        Vector2 p3(position.x, position.y + horizontal_line_height);

        immediate_quad(p0, p1, p2, p3, color);
    }

    {
        Vector2 offset = position + Vector2(0, horizontal_line_height + vertical_line_height);
        
        Vector2 p0(offset.x, offset.y);
        Vector2 p1(offset.x + horizontal_line_width, offset.y);
        Vector2 p2(offset.x + horizontal_line_width, offset.y + horizontal_line_height);
        Vector2 p3(offset.x, offset.y + horizontal_line_height);

        immediate_quad(p0, p1, p2, p3, color);
    }

    {
        Vector2 offset = position + Vector2(0, horizontal_line_height);
        
        Vector2 p0(offset.x, offset.y);
        Vector2 p1(offset.x + vertical_line_width, offset.y);
        Vector2 p2(offset.x + vertical_line_width, offset.y + vertical_line_height);
        Vector2 p3(offset.x, offset.y + vertical_line_height);

        immediate_quad(p0, p1, p2, p3, color);
    }

    {
        Vector2 offset = position + Vector2(horizontal_line_width - vertical_line_width, horizontal_line_height);
        
        Vector2 p0(offset.x, offset.y);
        Vector2 p1(offset.x + vertical_line_width, offset.y);
        Vector2 p2(offset.x + vertical_line_width, offset.y + vertical_line_height);
        Vector2 p3(offset.x, offset.y + vertical_line_height);
        
        immediate_quad(p0, p1, p2, p3, color);
    }
    
    immediate_flush();
}

void draw_editor() {
    set_render_targets(the_back_buffer, NULL);
    clear_color_target(the_back_buffer, 0.0f, 1.0f, 1.0f, 1.0f, globals.render_area);
    set_viewport(globals.render_area.x, globals.render_area.y, globals.render_width, globals.render_height);
    set_scissor(globals.render_area.x, globals.render_area.y, globals.render_width, globals.render_height);
    
    auto manager = get_entity_manager();
    set_matrix_for_entities(manager);
    refresh_global_parameters();
    draw_main_scene(manager);

    immediate_set_shader(globals.shader_color);
    if (has_currently_selected_tile) {
        draw_outline(currently_selected_tile);
    }
}
