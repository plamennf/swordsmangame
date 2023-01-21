#include "pch.h"
#include "game.h"
#include "os.h"
#include "render.h"
#include "font.h"
#include "draw_help.h"
#include "hud.h"
#include "binary_file_stuff.h"
#include "entity_manager.h"
#include "entities.h"
#include "animation.h"
#include "main_menu.h"
#include "camera.h"
#include "cursor.h"
#include "keymap.h"
#include "os.h"
#include "variable_service.h"
#include "editor.h"

#include "shader_registry.h"
#include "texture_registry.h"
#include "animation_registry.h"

#include <stdio.h>

#define ENABLE_SHADOWS 0

#define Attach(var) globals.variable_service->attach(#var, &var)

Game_Globals::Game_Globals() {
    variable_service = new Variable_Service();
    
    Attach(time_rate);
    Attach(zoom_speed);
}

const double GAMEPLAY_DT = 1.0 / 60.0; // @Hardcode

static bool save_current_game_mode();

static void keymap_do_hotloading() {
    u64 modtime = globals.keymap->modtime;
    get_file_last_write_time("data/Game.keymap", &modtime);

    if (modtime != globals.keymap->modtime) {
        if (!load_keymap(globals.keymap, "data/Game.keymap")) {
            set_keys_to_default(globals.keymap);
            log_error("Failed to load the game keymap. Setting all keys to their defaults\n");
        }
        globals.keymap->modtime = modtime;
    }
}

static void vars_do_hotloading() {
    u64 modtime = globals.variable_service->modtime;
    get_file_last_write_time("data/All.vars", &modtime);

    if (modtime != globals.variable_service->modtime) {
        load_vars_file(globals.variable_service, "data/All.vars"); // @ReturnValueIgnored
        globals.variable_service->modtime = modtime;
    }
}

float move_toward(float a, float b, float amount) {
    if (a > b) {
        a -= amount;
        if (a < b) a = b;
    } else {
        a += amount;
        if (a > b) a = b;
    }

    return a;
}

Game_Globals globals;
Global_Parameters global_parameters;

static Key_Info key_infos[NUM_KEY_CODES];

bool is_key_down(int key_code) {
    return key_infos[key_code].is_down;
}

bool is_key_pressed(int key_code) {
    return key_infos[key_code].is_down && key_infos[key_code].changed;
}

bool was_key_just_released(int key_code) {
    return key_infos[key_code].was_down && !key_infos[key_code].is_down;
}

bool is_key_down(Key_Action action) {
    if (action.alt_down && !is_key_down(KEY_ALT)) {
        return false;
    }

    if (action.shift_down && !is_key_down(KEY_SHIFT)) {
        return false;
    }

    if (action.ctrl_down && !is_key_down(KEY_CTRL)) {
        return false;
    }

    if (!is_key_down(action.key_code)) {
        return false;
    }

    return true;
}

bool is_key_pressed(Key_Action action) {
    if (action.alt_down && !is_key_down(KEY_ALT)) {
        return false;
    }

    if (action.shift_down && !is_key_down(KEY_SHIFT)) {
        return false;
    }

    if (action.ctrl_down && !is_key_down(KEY_CTRL)) {
        return false;
    }

    if (!is_key_pressed(action.key_code)) {
        return false;
    }

    return true;    
}

bool was_key_just_released(Key_Action action) {
    if (action.alt_down && !is_key_down(KEY_ALT)) {
        return false;
    }

    if (action.shift_down && !is_key_down(KEY_SHIFT)) {
        return false;
    }

    if (action.ctrl_down && !is_key_down(KEY_CTRL)) {
        return false;
    }

    if (!was_key_just_released(action.key_code)) {
        return false;
    }

    return true;
}

static double accumulated_dt = 0.0;

static void respond_to_event_for_game(Event event) {
    auto manager = get_entity_manager();
    Camera *camera = manager->camera;
    
    if (event.type == EVENT_TYPE_MOUSE_WHEEL) {
        if (camera) camera->handle_zoom(event.delta);
    }
}

static void update_single_guy(Guy *guy, Entity_Manager *manager, float dt) {
    Vector2 move_dir(0.0f, 0.0f);
    if (is_key_down(globals.keymap->move_left)) move_dir.x -= 1.0f;
    if (is_key_down(globals.keymap->move_right)) move_dir.x += 1.0f;

    if (is_key_down(globals.keymap->move_up)) move_dir.y += 1.0f;
    if (is_key_down(globals.keymap->move_down)) move_dir.y -= 1.0f;
    move_dir = normalize_or_zero(move_dir);
    
    float speed = 2.0f;    
    
    guy->max_velocity = Vector2(1.0f, 1.0f);
    
    guy->velocity.x = move_toward(guy->velocity.x, 0.0f, fabsf(guy->velocity.x) * 2.0f);
    guy->velocity.y = move_toward(guy->velocity.y, 0.0f, fabsf(guy->velocity.y) * 2.0f);
    guy->velocity.x += move_dir.x * speed * dt;
    guy->velocity.y += move_dir.y * speed * dt;
    
    Clamp(guy->velocity.x, -guy->max_velocity.x, guy->max_velocity.x);
    Clamp(guy->velocity.y, -guy->max_velocity.y, guy->max_velocity.y);

    Rectangle2 player_rect;
    player_rect.x = guy->position.x;
    player_rect.y = guy->position.y;
    player_rect.width = 0.9f;
    player_rect.height = 0.95f;

    Tilemap *tm = manager->tilemap;
    if (tm) {
        float xpos = tm->position.x;
        float ypos = tm->position.y;
        for (int y = 0; y < tm->height; y++) {
            for (int x = 0; x < tm->width; x++) {
                Tile tile = tm->tiles[y * tm->width + x];
                if (tile.is_collidable) {
                    Rectangle2 tile_rect;
                    tile_rect.x = xpos;
                    tile_rect.y = ypos;
                    tile_rect.width = 1.0f;
                    tile_rect.height = 1.0f;
                    
                    bool has_collided = false;
                    
                    if ((guy->velocity.x > 0.0f && is_touching_left(player_rect, tile_rect, guy->velocity)) || (guy->velocity.x < 0.0f && is_touching_right(player_rect, tile_rect, guy->velocity))) {
                        guy->velocity.x = 0.0f;
                        has_collided = true;
                    }
                    
                    if ((guy->velocity.y > 0.0f && is_touching_bottom(player_rect, tile_rect, guy->velocity)) || (guy->velocity.y < 0.0f && is_touching_top(player_rect, tile_rect, guy->velocity))) {
                        guy->velocity.y = 0.0f;
                        has_collided = true;
                    }
                    
                    if (has_collided) break;
                }
                xpos += 1.0f;
            }
            xpos = tm->position.x;
            ypos += 1.0f;
        }
    }

    guy->position += guy->velocity;// * dt;

    Guy_State state = guy->current_state;
    Guy_Orientation orientation = guy->orientation;
    
    if (guy->velocity.x != 0.0f || guy->velocity.y != 0.0f) {
        state = GUY_STATE_MOVING;

        if (guy->velocity.x > 0.0f) orientation = GUY_LOOKING_RIGHT;
        else if (guy->velocity.x < 0.0f) orientation = GUY_LOOKING_LEFT;
        
        if (guy->velocity.y > 0.0f) orientation = GUY_LOOKING_UP;
        else if (guy->velocity.y < 0.0f) orientation = GUY_LOOKING_DOWN;
    } else {
        state = GUY_STATE_IDLE;
    }
        
    guy->set_state(state);
    guy->set_orientation(orientation);

    auto light_source_e = manager->get_entity_by_id(guy->light_source_id);
    if (light_source_e) {
        auto light_source = (Light_Source *)light_source_e;
        light_source->position = guy->position + (0.5f * guy->size);
    }
}

static void simulate_game() {
    float dt = get_gameplay_dt();
    
    auto manager = get_entity_manager();
    manager->camera->update(dt);
    
    Guy *guy = manager->get_active_hero();
    if (guy) {
        update_single_guy(guy, manager, dt);
    }
    
    for (Thumbleweed *tw : manager->by_type._Thumbleweed) tw->update_current_animation(dt);
    for (Guy *guy : manager->by_type._Guy) guy->update_current_animation(dt);
}

static void respond_to_input() {
    if (is_key_pressed(globals.keymap->save_current_game_mode)) {
        save_current_game_mode();
        log("Current game mode saved successfully.\n");
    }
    
    if (is_key_pressed(KEY_ESCAPE)) {
        if (globals.program_mode == PROGRAM_MODE_GAME || globals.program_mode == PROGRAM_MODE_MENU) {
            toggle_menu();
        }
    }

    if (is_key_pressed(globals.keymap->toggle_fullscreen)) {
        window_toggle_fullscreen(globals.my_window);
    }
}

static void update_time(float dt_max) {
    double now = get_time();
    double delta = now - globals.last_time;
    float dilated_dt = (float)(delta * globals.time_rate);
    
    accumulated_dt += delta;
    
    float clamped_dilated_dt = dilated_dt;
    if (clamped_dilated_dt > dt_max) clamped_dilated_dt = dt_max;

    globals.time_info.current_dt = clamped_dilated_dt;
    globals.time_info.current_time += clamped_dilated_dt;
    
    globals.time_info.real_world_dt = dilated_dt;
    globals.time_info.real_world_time += dilated_dt;

    globals.time_info.ui_dt = (float)delta;
    globals.time_info.ui_time = (float)delta;
    
    globals.last_time = now;
}

static void init_shaders() {
    globals.shader_color = globals.shader_registry->get("color");
    globals.shader_texture = globals.shader_registry->get("texture");
    globals.shader_text = globals.shader_registry->get("text");
    globals.shader_guy = globals.shader_registry->get("guy");
    globals.shader_tile = globals.shader_registry->get("tile");
    globals.shader_lightmap_fx = globals.shader_registry->get("lightmap_fx");
    globals.shader_light = globals.shader_registry->get("light");
    globals.shader_shadow_segments = globals.shader_registry->get("shadow_segments");
    globals.shader_alpha_clear = globals.shader_registry->get("alpha_clear");
}

static void init_game() {
    globals.keymap = new Keymap();
    if (!load_keymap(globals.keymap, "data/Game.keymap")) {
        set_keys_to_default(globals.keymap);
        log_error("Failed to load the game keymap. Setting all keys to their defaults\n");
    }
    
    globals.current_game_mode = load_game_mode(GAME_MODE_OVERWORLD);
}

void set_matrix_for_entities(Entity_Manager *manager) {
    Tilemap *tm = manager->tilemap;
    Camera *camera = manager->camera;
    
    float half_width = 0.5f * tm->width;
    float half_height = 0.5f  * tm->height;
    
    global_parameters.proj_matrix = make_orthographic(-half_width * camera->zoom_t, half_width * camera->zoom_t, -half_height * camera->zoom_t, half_height * camera->zoom_t);
    global_parameters.view_matrix = camera->get_matrix();
    global_parameters.transform = global_parameters.proj_matrix * global_parameters.view_matrix;    
}

void draw_main_scene(Entity_Manager *manager) {
    //
    // Draw tilemaps
    //
    {
        Texture *last_texture = NULL;
        
        immediate_set_shader(globals.shader_tile);
        immediate_begin();
        Tilemap *tm = manager->tilemap;
        if (tm) {
            if (globals.render_type == RENDER_TYPE_LIGHTS && tm->flags & EF_CAN_CAST_SHADOWS ||
                globals.render_type == RENDER_TYPE_MAIN) {
                float xpos = tm->position.x;
                float ypos = tm->position.y;// + (tm->height - 1);
                
                for (int y = 0; y < tm->height; y++) {
                    for (int x = 0; x < tm->width; x++) {
                        Tile *tile = &tm->tiles[y * tm->width + x];
                        if (tile->id) {
                            Texture *texture = tm->textures[tile->id-1];
                            if (texture != last_texture) {
                                immediate_flush();
                                set_texture(0, texture);
                                last_texture = texture;
                            }
                        
                            Vector2 position(xpos, ypos);
                            Vector2 size(1, 1);

                            float hw = size.x * 0.5f;
                            float hh = size.y * 0.5f;

                            Vector2 center(position.x + hw, position.y + hh);
                        
                            Vector2 p0 = center + Vector2(-hw, -hh);
                            Vector2 p1 = center + Vector2(+hw, -hh);
                            Vector2 p2 = center + Vector2(+hw, +hh);
                            Vector2 p3 = center + Vector2(-hw, +hh);
                            
                            Vector2 uv0(0, 0);
                            Vector2 uv1(1, 0);
                            Vector2 uv2(1, 1);
                            Vector2 uv3(0, 1);

                            Vector4 color(1, 1, 1, 1);
                            if (globals.render_type == RENDER_TYPE_LIGHTS) {
                                color = Vector4(1, 1, 1, 0);
                            }
                        
                            immediate_quad(p0, p1, p2, p3, uv0, uv1, uv2, uv3, color);
                        }
                        xpos += 1.0f;
                    }
                    xpos = tm->position.x;
                    //ypos -= 1.0f;
                    ypos += 1.0f;
                }
            }
            immediate_flush();
        }
    }
        
    // @TODO: Collapse drawing guys and thumbleweed into one function
    // and add enemies to them after adding animations to them
    
    //
    // Draw thumbleweeds
    //
    immediate_set_shader(globals.shader_guy);
    for (Thumbleweed *tw : manager->by_type._Thumbleweed) {
        if (globals.render_type == RENDER_TYPE_LIGHTS && !(tw->flags & EF_CAN_CAST_SHADOWS)) continue;
        
        Animation *animation = tw->current_animation;
        if (!animation) continue;
        
        Texture *texture = animation->get_current_frame();
        if (!texture) continue;
        
        set_texture(0, texture);
        
        Vector2 position = tw->position;
        Vector2 size = tw->size;
        
        float hw = size.x * 0.5f;
        float hh = size.y * 0.5f;
        
        Vector2 center = position + Vector2(hw, hh);

        Vector2 p0 = center + Vector2(-hw, -hh);
        Vector2 p1 = center + Vector2(+hw, -hh);
        Vector2 p2 = center + Vector2(+hw, +hh);
        Vector2 p3 = center + Vector2(-hw, +hh);

        Vector2 uv0(0, 0);
        Vector2 uv1(1, 0);
        Vector2 uv2(1, 1);
        Vector2 uv3(0, 1);

        Vector4 color(1, 1, 1, 1);
        if (globals.render_type == RENDER_TYPE_LIGHTS) {
            color = Vector4(1, 1, 1, 0);
        }
        
        immediate_begin();
        immediate_quad(p0, p1, p2, p3, uv0, uv1, uv2, uv3, color);
        immediate_flush();
    }
    
    //
    // Draw enemies
    //
    immediate_set_shader(globals.shader_guy);
    for (Enemy *enemy : manager->by_type._Enemy) {
        if (globals.render_type == RENDER_TYPE_LIGHTS && !(enemy->flags & EF_CAN_CAST_SHADOWS)) continue;
        
        Texture *texture = enemy->texture;
        if (!texture) continue;
        
        set_texture(0, texture);
        
        Vector2 position = enemy->position;
        Vector2 size = enemy->size;
        
        float hw = size.x * 0.5f;
        float hh = size.y * 0.5f;
        
        Vector2 center = position + Vector2(hw, hh);

        Vector2 p0 = center + Vector2(-hw, -hh);
        Vector2 p1 = center + Vector2(+hw, -hh);
        Vector2 p2 = center + Vector2(+hw, +hh);
        Vector2 p3 = center + Vector2(-hw, +hh);
        
        Vector2 uv0(0, 0);
        Vector2 uv1(1, 0);
        Vector2 uv2(1, 1);
        Vector2 uv3(0, 1);

        Vector4 color(1, 1, 1, 1);
        if (globals.render_type == RENDER_TYPE_LIGHTS) {
            color = Vector4(1, 1, 1, 0);
        }
        
        immediate_begin();
        immediate_quad(p0, p1, p2, p3, uv0, uv1, uv2, uv3, color);
        immediate_flush();
    }

    //
    // Draw guys
    //
    immediate_set_shader(globals.shader_guy);
    for (Guy *guy : manager->by_type._Guy) {
        if (globals.render_type == RENDER_TYPE_LIGHTS && !(guy->flags & EF_CAN_CAST_SHADOWS)) continue;
        
        Animation *animation = guy->current_animation;
        if (!animation) continue;
        
        Texture *texture = animation->get_current_frame();
        if (!texture) continue;
        
        set_texture(0, texture);
        
        Vector2 position = guy->position;
        Vector2 size = guy->size;
        
        float hw = size.x * 0.5f;
        float hh = size.y * 0.5f;
        
        Vector2 center = position + Vector2(hw, hh);

        Vector2 p0 = center + Vector2(-hw, -hh);
        Vector2 p1 = center + Vector2(+hw, -hh);
        Vector2 p2 = center + Vector2(+hw, +hh);
        Vector2 p3 = center + Vector2(-hw, +hh);

        Vector2 uv0(0, 0);
        Vector2 uv1(1, 0);
        Vector2 uv2(1, 1);
        Vector2 uv3(0, 1);

        Vector4 color(1, 1, 1, 1);
        if (globals.render_type == RENDER_TYPE_LIGHTS) {
            color = Vector4(1, 1, 1, 0);
        }
        
        immediate_begin();
        immediate_quad(p0, p1, p2, p3, uv0, uv1, uv2, uv3, color);
        immediate_flush();
    }
}

void resolve_to_screen() {
    set_render_targets(the_back_buffer, NULL);
    clear_color_target(the_back_buffer, 0.0f, 0.0f, 0.0f, 1.0f);
    set_viewport(0, 0, globals.display_width, globals.display_height);
    set_scissor(0, 0, globals.display_width, globals.display_height);

    rendering_2d_right_handed(globals.display_width, globals.display_height);
    refresh_global_parameters();
    
    immediate_set_shader(globals.shader_lightmap_fx);
    set_texture(0, the_offscreen_buffer->texture);
    set_texture(1, the_lightmap_buffer->texture);
    
    float x0 = (globals.display_width - globals.render_width) * 0.5f;
    float y0 = (globals.display_height - globals.render_height) * 0.5f;
    float x1 = x0 + (float)globals.render_width;
    float y1 = y0 + (float)globals.render_height;

#ifdef RENDER_D3D11
    // Use flipped-y uvs because d3d11 uses left-hand coordinate systems.
    Vector2 uv0(0, 1);
    Vector2 uv1(1, 1);
    Vector2 uv2(1, 0);
    Vector2 uv3(0, 0);
#else
    Vector2 uv0(0, 0);
    Vector2 uv1(1, 0);
    Vector2 uv2(1, 1);
    Vector2 uv3(0, 1);
#endif
    
    Vector4 color(1, 1, 1, 1);
    
    immediate_begin();
    immediate_quad(x0, y0, x1, y1, uv0, uv1, uv2, uv3, color);
    immediate_flush();
}

static void draw_circle(Vector2 center, float radius, Vector4 color) {
    const int NUM_TRIANGLES = 100;
    auto dtheta = TAU / NUM_TRIANGLES;
    float r = radius;

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
}

// @CopyPaste from draw_outline_around_entity
static void draw_shadow_segments(Entity *e) {
    Vector4 color(0, 0, 0, 0);

    Vector2 p0 = e->position;
    Vector2 p1 = e->position + Vector2(e->size.x, 0);
    Vector2 p2 = e->position + e->size;
    Vector2 p3 = e->position + Vector2(0, e->size.y);

    {
        Vector3 a0(p0, 0);
        Vector3 a1(p0, 1);
        Vector3 b0(p1, 0);
        Vector3 b1(p1, 1);

        immediate_quad(a0, a1, b0, b1, color);
    }
    
    {
        Vector3 a0(p1, 0);
        Vector3 a1(p1, 1);
        Vector3 b0(p2, 0);
        Vector3 b1(p2, 1);

        immediate_quad(a0, a1, b0, b1, color);
    }

    {
        Vector3 a0(p2, 0);
        Vector3 a1(p2, 1);
        Vector3 b0(p3, 0);
        Vector3 b1(p3, 1);

        immediate_quad(a0, a1, b0, b1, color);
    }

    {
        Vector3 a0(p3, 0);
        Vector3 a1(p3, 1);
        Vector3 b0(p0, 0);
        Vector3 b1(p0, 1);

        immediate_quad(a0, a1, b0, b1, color);
    }
}

static Vector4 to_vec4(Vector3 v) {
    return Vector4(v.x, v.y, v.z, 1.0f);
}

static void draw_lights() {
    auto manager = get_entity_manager();
    set_matrix_for_entities(manager);
    refresh_global_parameters();
    
    for (Light_Source *source : manager->by_type._Light_Source) {
#if ENABLE_SHADOWS
        immediate_set_shader(globals.shader_shadow_segments);
        global_parameters.light_position = source->position;
        refresh_global_parameters();
        for (Entity *e : manager->all_entities) {
            if (!(e->flags & EF_CAN_CAST_SHADOWS)) continue;
            draw_shadow_segments(e);
        }
#endif
        
        immediate_set_shader(globals.shader_light);
        immediate_begin();
        draw_circle(source->position, source->radius, to_vec4(source->color));
        immediate_flush();
    }
}

static void draw_one_frame() {
    set_render_targets(the_lightmap_buffer, NULL);
    clear_color_target(the_lightmap_buffer, 19/255.0f, 24/255.0f, 98/255.0f, 1.0f);
    set_viewport(0, 0, globals.render_width, globals.render_height);
    set_scissor(0, 0, globals.render_width, globals.render_height);

    globals.render_type = RENDER_TYPE_LIGHTS;
    draw_lights();
    
    set_render_targets(the_offscreen_buffer, NULL);
    clear_color_target(the_offscreen_buffer, 0.0f, 1.0f, 1.0f, 1.0f);
    set_viewport(0, 0, globals.render_width, globals.render_height);
    set_scissor(0, 0, globals.render_width, globals.render_height);
    
    auto manager = get_entity_manager();
    set_matrix_for_entities(manager);
    refresh_global_parameters();

    globals.render_type = RENDER_TYPE_MAIN;
    draw_main_scene(manager);

    resolve_to_screen();
    
    draw_hud();
}

static void do_one_frame() {
    reset_temporary_storage();

    update_time(0.15f);
    
    for (Window_Resize_Record record : globals.window_resizes) {
        int width = record.width;
        int height = record.height;
        
        render_resize(width, height);
        
        globals.display_width = width;
        globals.display_height = height;

        init_menu_fonts();
    }
    globals.window_resizes.count = 0;

    for (int i = 0; i < ArrayCount(key_infos); i++) {
        Key_Info *info = &key_infos[i];
        info->changed = false;
        info->was_down = info->is_down;
    }
    update_window_events();
    for (Event event : globals.events_this_frame) {
        switch (event.type) {
        case EVENT_TYPE_QUIT:
            globals.should_quit_game = true;
            break;

        case EVENT_TYPE_KEYBOARD: {
            Key_Info *info = &key_infos[event.key_code];
            info->changed = event.key_pressed != info->is_down;
            info->is_down = event.key_pressed;

            if (event.alt_pressed && event.key_pressed) {
                if (event.key_code == KEY_F4) {
                    globals.should_quit_game = true;
                }
            }
            
            break;
        }

        case EVENT_TYPE_WINDOW_FOCUS:
            globals.app_is_focused = event.has_received_focus;
            break;
        }

        if (globals.program_mode == PROGRAM_MODE_GAME) {
            respond_to_event_for_game(event);
        }
    }
    
    {
        auto manager = get_entity_manager();
        Tilemap *tilemap = manager->tilemap;
        globals.render_area = aspect_ratio_fit(globals.display_width, globals.display_height, tilemap->width, tilemap->height);
        
        globals.render_width = globals.render_area.width;
        globals.render_height = globals.render_area.height;
        
        globals.world_space_size_x = tilemap->width;
        globals.world_space_size_y = tilemap->height;

        if (!the_lightmap_buffer) {
            the_lightmap_buffer = create_color_target(globals.render_width, globals.render_height);
        }

        if (!the_offscreen_buffer) {
            the_offscreen_buffer = create_color_target(globals.render_width, globals.render_height);
        }
    }
    
    respond_to_input();

    if (globals.app_is_focused) {
        os_show_cursor(false);
    } else {
        os_show_cursor(true);
    }

    if (is_key_pressed(globals.keymap->toggle_editor)) {
        toggle_editor();
    }

    os_show_cursor(true);
    
    globals.draw_cursor = false;
    if (globals.program_mode == PROGRAM_MODE_GAME) {
        if (globals.app_is_focused) {
            //os_constrain_mouse(globals.my_window);
        } else {
            os_unconstrain_mouse();
        }
        globals.draw_cursor = false;
        
        while (accumulated_dt >= GAMEPLAY_DT) {
            simulate_game();
            accumulated_dt -= GAMEPLAY_DT;
        }
    } else {
        accumulated_dt = 0.0;
        os_unconstrain_mouse();
        globals.draw_cursor = true;
        
        if (globals.program_mode == PROGRAM_MODE_MENU) {
            update_menu();
        } else if (globals.program_mode == PROGRAM_MODE_EDITOR) {
            update_editor();
        }
    }
    
    if (globals.program_mode == PROGRAM_MODE_GAME) {
        draw_one_frame();
    } else if (globals.program_mode == PROGRAM_MODE_MENU) {
        draw_menu();
    } else if (globals.program_mode == PROGRAM_MODE_EDITOR) {
        draw_editor();
    }
    
    if (globals.camera_is_moving) {
        globals.draw_cursor = true;
    }
    
    if (globals.draw_cursor) {
        Cursor_Type cursor_type = CURSOR_TYPE_DOT;
        if (globals.camera_is_moving) {
            cursor_type = CURSOR_TYPE_FOUR_ARROWS;
        }
        
        draw_cursor(globals.camera_is_moving, cursor_type);
    }
    
    swap_buffers();
    
    globals.shader_registry->do_hotloading();
    globals.texture_registry->do_hotloading();
    globals.animation_registry->do_hotloading();
    keymap_do_hotloading();
    vars_do_hotloading();
}

static void main_loop() {
    while (!globals.should_quit_game) {
        do_one_frame();
    }
}

int main(int argc, char **argv) {
    init_temporary_storage(40000);
    init_colors_and_utf8();

    {
        char *path = get_path_of_running_executable();
        defer { delete [] path; };
        
        char *slash = strrchr(path, '/');
        path[slash - path] = 0;

        setcwd(path);
    }

    
    
    globals.last_time = get_time();

    globals.display_width = 1600;
    globals.display_height = 900;
    globals.my_window = create_window(globals.display_width, globals.display_height, "Giovanni Yatsuro");
    init_render(globals.my_window, globals.display_width, globals.display_height, true);

    globals.shader_registry = new Shader_Registry();
    globals.texture_registry = new Texture_Registry();
    globals.animation_registry = new Animation_Registry();

    init_shaders();
    
    load_vars_file(globals.variable_service, "data/All.vars"); // @ReturnValueIgnored
    
    init_game();
    
    main_loop();
    
    return 0;
}

float get_gameplay_dt() {
    return (float)((double)GAMEPLAY_DT * globals.time_rate);
}

static char *get_savegame_name_for_game_mode(Game_Mode game_mode) {
    switch (game_mode) {
    case GAME_MODE_OVERWORLD: return "overworld";
    }
    return NULL;
}

static void init_overworld(Game_Mode_Info *info);

static Game_Mode_Info *make_new_game_mode(Game_Mode game_mode) {
    Game_Mode_Info *info = new Game_Mode_Info();
    switch (game_mode) {
    case GAME_MODE_OVERWORLD: init_overworld(info);
    }
    return info;
}

Game_Mode_Info *load_game_mode(Game_Mode game_mode) {
    char *savegame_name = get_savegame_name_for_game_mode(game_mode);
    if (!savegame_name) return NULL;
    
    char *full_path = tprint("data/saves/%s.level", savegame_name);    
    FILE *file = fopen(full_path, "rb");
    if (!file) {
        log("Save file for level '%s' does not exist. Making a new one.\n", savegame_name);
        return make_new_game_mode(game_mode);
    }
    defer { fclose(file); };
    
    bool error = false;
    
    int version = -1;
    get_u2b(file, &version, &error);
    if (error) {
        log_error("File '%s' is too short be a valid file: Version number is missing.\n", full_path);
        return NULL;
    }

    if (version > GAME_MODE_FILE_VERSION) {
        log_error("Error in file '%s': Versoin number is too high. Current max file version is %d", GAME_MODE_FILE_VERSION);
        return NULL;
    }

    int num_entities = 0;
    get_u4b(file, &num_entities, &error);
    if (error) {
        log_error("File '%s' is too short be a valid file: The number of entities is missing.\n", full_path);
        return NULL;
    }
    
    Entity_Manager *manager = new Entity_Manager();
    
    for (int i = 0; i < num_entities; i++) {
        int entity_type;
        get_u1b(file, &entity_type, &error);
        if (error) {
            log_error("File '%s' is too short be a valid file: Entity type is missing.\n", full_path);
            delete manager;
            return NULL;
        }
        int id;
        get_u4b(file, &id, &error);
        if (error) {
            log_error("File '%s' is too short be a valid file: Entity id is missing.\n", full_path);
            delete manager;
            return NULL;
        }
        
        if (entity_type == ENTITY_TYPE_GUY) {
            Guy *guy = manager->make_guy();
            guy->id = id;

            get_vector2(file, &guy->position, &error);
            if (error) {
                log_error("File '%s' is too short be a valid file: Entity position is missing.\n", full_path);
                delete manager;
                return NULL;
            }
            
            get_vector2(file, &guy->size, &error);
            if (error) {
                log_error("File '%s' is too short be a valid file: Guy size is missing.\n", full_path);
                delete manager;
                return NULL;
            }
        } else if (entity_type == ENTITY_TYPE_TILEMAP) {
            Tilemap *tm = manager->make_tilemap();

            get_vector2(file, &tm->position, &error);
            if (error) {
                log_error("File '%s' is too short be a valid file: Entity position is missing.\n", full_path);
                delete manager;
                return NULL;
            }
            
            char *name;
            get_string(file, &name, &error);
            if (error) {
                log_error("File '%s' is too short be a valid file: Tilemap name is missing.\n", full_path);
                delete manager;
                return NULL;
            }

            load_tilemap(tm, name);
        }
    }

    get_u4b(file, &manager->next_entity_id, &error);
    if (error) {
        log_error("File '%s' is too short to be a valid file: Entity manager next_entity_id is missing.\n", full_path);
        delete manager;
        return NULL;
    }
    
    Game_Mode_Info *info = new Game_Mode_Info();
    info->savegame_name = savegame_name; // All strings should be constant so we don't bother copying it.
    info->game_mode = game_mode;
    info->entity_manager = manager;
    
    return info;
}

// @Incomplete:
// - Add enemy
static bool save_current_game_mode() {
    auto info = globals.current_game_mode;
    auto manager = info->entity_manager;

    // TODO: Make sure the save directory exists
    
    char *full_path = tprint("data/saves/%s.level", info->savegame_name);
    FILE *file = fopen(full_path, "wb");
    if (!file) {
        log_error("Failed to open file '%s' for writing.\n", full_path);
        return false;
    }
    defer { fclose(file); };

    put_u2b(GAME_MODE_FILE_VERSION, file);

    Array <Entity *> entities;
    for (int i = 0; i < manager->entity_lookup.allocated; i++) {
        if (manager->entity_lookup.occupancy_mask[i]) {
            entities.add(manager->entity_lookup.buckets[i].value);
        }
    }

    put_u4b(entities.count, file);

    for (int i = 0; i < entities.count; i++) {
        Entity *e = entities[i];
        
        put_u1b((u8)e->type, file);
        put_u4b(e->id, file);

        put_vector2(&e->position, file);
        
        if (e->type == ENTITY_TYPE_GUY) {
            Guy *guy = (Guy *)e;
            put_vector2(&guy->size, file);
        } else if (e->type == ENTITY_TYPE_TILEMAP) {
            Tilemap *tm = (Tilemap *)e;
            put_string(tm->name, file);
        }
    }

    put_u4b(manager->next_entity_id, file);

    return true;
}

static void init_overworld(Game_Mode_Info *info) {
    info->game_mode = GAME_MODE_OVERWORLD;
    info->entity_manager = new Entity_Manager();
    info->savegame_name = get_savegame_name_for_game_mode(GAME_MODE_OVERWORLD);
    
    auto manager = info->entity_manager;
    
    Camera *camera = new Camera();
    camera->position = Vector2(0, 0);
    camera->zoom_t_target = 1.0f;
    camera->zoom_t = 1.0f;
    manager->camera = camera;
    
    Guy *guy = manager->make_guy();
    guy->position = Vector2(0.0f, 0.5f);
    guy->size = Vector2(1.0f, 1.0f);
    guy->flags = 0;
    manager->set_active_hero(guy);
    
    Tilemap *tilemap = manager->make_tilemap();
    load_tilemap(tilemap, "test");
    tilemap->position = Vector2(-8.0f, -4.5f);

    Enemy *enemy = manager->make_enemy();
    enemy->position = Vector2(-7.0f, -3.5f);
    enemy->texture = globals.texture_registry->get("pachi_demon_knight_front");

    Thumbleweed *thumbleweed = manager->make_thumbleweed();
    thumbleweed->position = Vector2(-1.0f, -0.5f);

    // TEMPORARY
    thumbleweed->current_animation = thumbleweed->attack_animation;

    Light_Source *source = manager->make_light_source();
    source->position = guy->position;
    source->radius = 1.0f;
    source->color = Vector3(1.0f, 0.5f, 0.2f);
    guy->light_source_id = source->id;
}

Entity_Manager *get_entity_manager() {
    auto info = globals.current_game_mode;
    return info->entity_manager;
}

void toggle_editor() {
    if (globals.program_mode == PROGRAM_MODE_GAME) {
        globals.program_mode = PROGRAM_MODE_EDITOR;
    } else if (globals.program_mode = PROGRAM_MODE_EDITOR) {
        globals.program_mode = PROGRAM_MODE_GAME;
    }
}

Vector2 screen_space_to_world_space(int x, int y, bool is_position) {
    float fx = (float)x;
    float fy = (float)y;
    
    fx /= globals.display_width;
    fy /= globals.display_height;

    fx *= globals.world_space_size_x;
    fy *= globals.world_space_size_y;

    if (is_position) {
        fx -= globals.world_space_size_x * 0.5f;
        fy -= globals.world_space_size_y * 0.5f;
    }

    return Vector2(fx, fy);
}
