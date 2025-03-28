#include "pch.h"
#include "editor.h"
#include "render.h"
#include "draw.h"
#include "game.h"
#include "os.h"
#include "entities.h"
#include "entity_manager.h"

static int currently_selected_entity_id = -1;
static int mouse_x_last_frame;
static int mouse_y_last_frame;
static int mouse_x_offset;
static int mouse_y_offset;

void update_editor() {
    {
        int mx, my;
        os_get_mouse_pointer_position(&mx, &my, globals.my_window, true);

        mouse_x_offset = mx - mouse_x_last_frame;
        mouse_y_offset = my - mouse_y_last_frame;

        mouse_x_last_frame = mx;
        mouse_y_last_frame = my;
    }
    
    auto manager = get_entity_manager();
    
    if (is_key_pressed(MOUSE_BUTTON_LEFT)) {
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

        nx -= globals.world_space_size_x * 0.5f;
        ny -= globals.world_space_size_y * 0.5f;

        int entity_id = -1;

        for (int i = manager->all_entities.count-1; i >= 0; i--) {
            Entity *e = manager->all_entities[i];
            if (nx >= e->position.x && ny >= e->position.y &&
                nx <= e->position.x + e->size.x && ny <= e->position.y + e->size.y) {
                entity_id = e->id;
                break;
            }
        }
        
        currently_selected_entity_id = entity_id;
    }

    if (currently_selected_entity_id != -1) {
        if (is_key_down(MOUSE_BUTTON_LEFT) && (mouse_x_offset || mouse_y_offset)) {
            Entity *e = manager->get_entity_by_id(currently_selected_entity_id);
            if (!e) return;
            
            Vector2 mpos = screen_space_to_world_space(mouse_x_offset, mouse_y_offset, false);
            e->position += mpos;
        }
    }

    if (is_key_down(KEY_CTRL)) {
        if (is_key_pressed('V')) {
            auto e = manager->get_entity_by_id(currently_selected_entity_id);
            if (e) {
                auto copied = manager->add_entity(e);
                currently_selected_entity_id = copied->id;
            }
        }
    }
}

static void draw_outline_around_entity(int entity_id) {
    auto manager = get_entity_manager();
    Entity *e = manager->get_entity_by_id(entity_id);
    if (!e) return;
    
    Vector4 color(0, 1, 0, 1);

    Vector2 position = e->position;
    
    float horizontal_line_width = e->size.x;//1.0f;
    float horizontal_line_height = 0.01f;
    float vertical_line_width = 0.01f;
    //float vertical_line_height = 1.0f - 2.0f*horizontal_line_height;
    float vertical_line_height = e->size.y - 2.0f*horizontal_line_height;
    
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
    set_render_targets(the_offscreen_buffer, NULL);
    clear_color_target(the_offscreen_buffer, 0.0f, 1.0f, 1.0f, 1.0f);
    set_viewport(0, 0, globals.render_width, globals.render_height);
    set_scissor(0, 0, globals.render_width, globals.render_height);
    
    auto manager = get_entity_manager();
    set_matrix_for_entities(manager);
    refresh_global_parameters();
    draw_main_scene(manager);
    
    set_shader(globals.shader_color);
    if (currently_selected_entity_id != -1) {
        draw_outline_around_entity(currently_selected_entity_id);
    }
    
    resolve_to_screen();
}
