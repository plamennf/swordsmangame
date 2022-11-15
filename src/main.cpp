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

#include "shader_registry.h"
#include "texture_registry.h"
#include "animation_registry.h"

#include <stdio.h>

const double GAMEPLAY_DT = 1.0 / 60.0; // @Hardcode

static bool save_current_game_mode();

// @Speed
static bool collidable_tiles_in_region(Vector2 pos, Vector2 size) {
    auto manager = get_entity_manager();

    int ix, iy;

    float threshold = 0.01f;
    
    float fract_x = fract(pos.x);
    float inv_fract_x = 1.0f - fract_x;
    if (inv_fract_x < threshold) {
        ix = (int)(pos.x + 0.5f);
    } else {
        ix = (int)(floorf(pos.x));
    }

    float fract_y = fract(pos.y);
    float inv_fract_y = 1.0f - fract_y;
    if (inv_fract_y < threshold) {
        iy = (int)(pos.y + 0.5f);
    } else {
        iy = (int)(floorf(pos.y));
    }
    
    //int ix = (int)(floorf(pos.x));
    //int iy = (int)(floorf(pos.y));

    float a1 = pos.x;
    float a2 = pos.x + size.x;
    float b1 = pos.y;
    float b2 = pos.y + size.y;
    
    for (Tilemap *tm : manager->by_type._Tilemap) {
        float xpos = tm->position.x;
        float ypos = tm->position.y;// + (tm->height - 1);
        for (int y = 0; y < tm->height; y++) {
            for (int x = 0; x < tm->width; x++) {
                int tix = (int)xpos;
                int tiy = (int)ypos;
                if ((xpos > a1 && xpos < a2 && ypos > b1 && ypos < b2) ||
                    (ix == tix && iy == tiy)) {
                    Tile *tile = &tm->tiles[y * tm->width + x];
                    if (tile->is_collidable) {
                        return true;
                    }
                }
                
                xpos += 1.0f;
            }
            xpos = tm->position.x;
            //ypos -= 1.0f;
            ypos += 1.0f;
        }
    }

    return false;
}

static float move_toward(float a, float b, float amount) {
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

static double accumulated_dt = 0.0;

static void simulate_game() {
    float dt = get_gameplay_dt();

    auto manager = get_entity_manager();
    
    Vector2 move_dir(0.0f, 0.0f);
    if (is_key_down('A')) move_dir.x -= 1.0f;
    if (is_key_down('D')) move_dir.x += 1.0f;

    if (is_key_down('W')) move_dir.y += 1.0f;
    if (is_key_down('S')) move_dir.y -= 1.0f;
    move_dir = normalize_or_zero(move_dir);

    float speed = 0.5f;
    
    Guy *guy = manager->by_type._Guy[0];
    if (guy->current_animation) {
        guy->current_animation->update(dt);
    }
    
    guy->max_velocity = Vector2(1.0f, 1.0f);

    guy->velocity.x = move_toward(guy->velocity.x, 0.0f, fabsf(guy->velocity.x) * 2.0f);
    guy->velocity.y = move_toward(guy->velocity.y, 0.0f, fabsf(guy->velocity.y) * 2.0f);
    guy->velocity.x += move_dir.x * speed;
    guy->velocity.y += move_dir.y * speed;
    
    Clamp(guy->velocity.x, -guy->max_velocity.x, guy->max_velocity.x);
    Clamp(guy->velocity.y, -guy->max_velocity.y, guy->max_velocity.y);
    
    Vector2 ps[] = {
        { 1.0f, 1.0f },
        { 0.0f, 1.0f },
        { 1.0f, 0.0f },
    };
    
    const int MAX_ATTEMPTS = 8;
    for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
        for (int i = 0; i < ArrayCount(ps); i++) {
            Vector2 new_vel = componentwise_product(guy->velocity, ps[i]);
            Vector2 new_position = guy->position + new_vel * dt;
            if (!collidable_tiles_in_region(new_position, guy->size)) {
                guy->position = new_position;
                guy->velocity = new_vel;
                break;
            }
        }
    }

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
}

static void respond_to_input() {
    if (is_key_down(KEY_CTRL)) {
        if (is_key_pressed('P')) {
            save_current_game_mode();
            log("Current game mode saved successfully.\n");
        }
    }

    if (is_key_pressed(KEY_ESCAPE)) {
        if (globals.program_mode == PROGRAM_MODE_GAME || globals.program_mode == PROGRAM_MODE_MENU) {
            toggle_menu();
        }
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
}

static void init_game() {
    globals.current_game_mode = load_game_mode(GAME_MODE_OVERWORLD);
}

static void draw_one_frame() {
    set_render_targets(the_back_buffer, NULL);
    clear_color_target(the_back_buffer, 0.0f, 1.0f, 1.0f, 1.0f, globals.render_area);
    set_viewport(globals.render_area.x, globals.render_area.y, globals.render_width, globals.render_height);
    set_scissor(globals.render_area.x, globals.render_area.y, globals.render_width, globals.render_height);
    
    rendering_2d_right_handed(globals.render_width, globals.render_height);
    refresh_global_parameters();
    
    auto manager = get_entity_manager();

    //
    // Draw tilemaps
    //
    {
        Texture *last_texture = NULL;
        
        immediate_set_shader(globals.shader_tile);
        immediate_begin();
        for (Tilemap *tm : manager->by_type._Tilemap) {
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
                        
                        Vector2 position = world_space_to_screen_space(Vector2(xpos, ypos));
                        Vector2 size = world_space_to_screen_space(Vector2(1, 1));

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

    //
    // Draw guys
    //
    immediate_set_shader(globals.shader_guy);
    for (Guy *guy : manager->by_type._Guy) {
        Animation *animation = guy->current_animation;
        if (!animation) continue;
        
        Texture *texture = animation->get_current_frame();
        if (!texture) continue;
        
        set_texture(0, texture);
        
        Vector2 position = world_space_to_screen_space(guy->position);
        Vector2 size = world_space_to_screen_space(guy->size);
        
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
        
        immediate_begin();
        immediate_quad(p0, p1, p2, p3, uv0, uv1, uv2, uv3, Vector4(1, 1, 1, 1));
        immediate_flush();
    }

    //
    // Draw enemies
    //
    immediate_set_shader(globals.shader_guy);
    for (Enemy *enemy : manager->by_type._Enemy) {
        Texture *texture = enemy->texture;
        if (!texture) continue;
        
        set_texture(0, texture);
        
        Vector2 position = world_space_to_screen_space(enemy->position);
        Vector2 size = world_space_to_screen_space(enemy->size);
        
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
        
        immediate_begin();
        immediate_quad(p0, p1, p2, p3, uv0, uv1, uv2, uv3, Vector4(1, 1, 1, 1));
        immediate_flush();
    }
    
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
        }
    }

    {
        auto manager = get_entity_manager();
        Tilemap *tilemap = manager->by_type._Tilemap[0]; // @Hack
        globals.render_area = aspect_ratio_fit(globals.display_width, globals.display_height, tilemap->width, tilemap->height);
        
        globals.render_width = globals.render_area.width;
        globals.render_height = globals.render_area.height;

        globals.world_space_size_x = tilemap->width;
        globals.world_space_size_y = tilemap->height;
    }
    
    respond_to_input();

    if (globals.program_mode == PROGRAM_MODE_GAME) {
        while (accumulated_dt >= GAMEPLAY_DT) {
            simulate_game();
            accumulated_dt -= GAMEPLAY_DT;
        }
    } else {
        accumulated_dt = 0.0;

        if (globals.program_mode == PROGRAM_MODE_MENU) {
            update_menu();
        }
    }

    if (globals.program_mode == PROGRAM_MODE_GAME) {
        draw_one_frame();
    } else if (globals.program_mode == PROGRAM_MODE_MENU) {
        draw_menu();
    }
    
    swap_buffers();
    
    globals.shader_registry->do_hotloading();
    globals.texture_registry->do_hotloading();
    globals.animation_registry->do_hotloading();
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
    init_render(globals.my_window, globals.display_width, globals.display_height, false);

    globals.shader_registry = new Shader_Registry();
    globals.texture_registry = new Texture_Registry();
    globals.animation_registry = new Animation_Registry();

    init_shaders();
    
    init_game();
    
    main_loop();
    
    return 0;
}

float get_gameplay_dt() {
    return (float)GAMEPLAY_DT;
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
    Guy *guy = manager->make_guy();
    guy->position = Vector2(5.0f, 5.0f);
    guy->size = Vector2(1.0f, 1.0f);
    
    Tilemap *tilemap = manager->make_tilemap();
    load_tilemap(tilemap, "test");
    tilemap->position = Vector2(0, 0);

    Enemy *enemy = manager->make_enemy();
    enemy->position = Vector2(1, 1);
    enemy->texture = globals.texture_registry->get("pachi_demon_knight_front");
}

Vector2 world_space_to_screen_space(Vector2 v) {
    Vector2 result = v;
    
    result.x /= globals.world_space_size_x;//WORLD_SPACE_SIZE_X;
    result.y /= globals.world_space_size_y;//WORLD_SPACE_SIZE_Y;

    result.x *= globals.render_width;
    result.y *= globals.render_height;
    
    return result;
}

Entity_Manager *get_entity_manager() {
    auto info = globals.current_game_mode;
    return info->entity_manager;
}
