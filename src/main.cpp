#include "pch.h"
#include "game.h"
#include "os.h"
#include "render.h"
#include "font.h"
#include "draw.h"
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
#include "text_file_handler.h"

#define CUTE_C2_IMPLEMENTATION
#include <cute_c2.h>

#include "shader_registry.h"
#include "texture_registry.h"
#include "animation_registry.h"

#include <stdio.h>

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

    auto new_position = guy->position + guy->velocity;
    
    c2AABB player_aabb;
    player_aabb.min = { new_position.x, new_position.y };
    player_aabb.max = { new_position.x + guy->size.x * 0.9f, new_position.y + guy->size.y * 0.9f };

    Tilemap *tm = manager->tilemap;
    if (tm) {
        float xpos = tm->position.x;
        float ypos = tm->position.y;
        for (int y = 0; y < tm->height; y++) {
            for (int x = 0; x < tm->width; x++) {
                Tile tile = tm->tiles[y * tm->width + x];
                if (tile.is_collidable) {
                    c2AABB tile_aabb;
                    tile_aabb.min = { xpos, ypos };
                    tile_aabb.max = { xpos + 1.0f, ypos + 1.0f };
                    
                    bool has_collided = false;
                    
                    c2Manifold m;
                    c2AABBtoAABBManifold(player_aabb, tile_aabb, &m);
                    if (m.count) {
                        Vector2 n(m.n.x, m.n.y);
                        
                        if (n.x != 0.0f) {
                            guy->velocity.x = 0.0f;
                        }

                        if (n.y != 0.0f) {
                            guy->velocity.y = 0.0f;
                        }
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

        bool moving_diagonally = guy->velocity.x && guy->velocity.y;
        if (!moving_diagonally) {
            if (guy->velocity.x > 0.0f) orientation = GUY_LOOKING_RIGHT;
            else if (guy->velocity.x < 0.0f) orientation = GUY_LOOKING_LEFT;
            
            if (guy->velocity.y > 0.0f) orientation = GUY_LOOKING_UP;
            else if (guy->velocity.y < 0.0f) orientation = GUY_LOOKING_DOWN;
        }
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
        globals.draw_cursor = false;
        
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

static void load_guy(Guy *guy, FILE *file) {
    int is_active = 0;
    fscanf(file, "is_active %d\n", &is_active);
    guy->is_active = (bool)is_active;
    fscanf(file, "light_source_id %d\n", &guy->light_source_id);
}

static void load_enemy(Enemy *enemy, FILE *file) {
    
}

static void load_thumbleweed(Enemy *enemy, FILE *file) {
    
}

static void load_light_source(Light_Source *ls, FILE *file) {
    fscanf(file, "radius %f\n", &ls->radius);
    fscanf(file, "color (%f, %f, %f)\n", &ls->color.x, &ls->color.y, &ls->color.z);
}

static void load_tree(Tree *tree, FILE *file) {
    
}

Game_Mode_Info *load_game_mode(Game_Mode game_mode) {
    char *savegame_name = get_savegame_name_for_game_mode(game_mode);
    if (!savegame_name) return NULL;
    
    char *full_path = tprint("data/saves/%s/%s.level_info", savegame_name, savegame_name);
    FILE *file = fopen(full_path, "rt");
    if (!file) {
        log("Save file for level '%s' does not exist. Making a new one.\n", savegame_name);
        return make_new_game_mode(game_mode);
    }
    defer { fclose(file); };

    auto manager = new Entity_Manager();
    
    Game_Mode_Info *info = new Game_Mode_Info();
    info->savegame_name = savegame_name; // All strings should be constant so we don't bother copying it.
    info->game_mode = game_mode;
    info->entity_manager = manager;

    Text_File_Handler handler;
    handler.start_file(full_path, full_path, "load_game_mode");

    char tilemap_name[4096] = {};    
    {
        char *line = handler.consume_next_line();
        sscanf(line, "tilemap %s", tilemap_name);
    }
    
    while (true) {
        char *file_name = handler.consume_next_line();
        if (!file_name) break;

        FILE *file = fopen(file_name, "rt");
        if (!file) {
            log_error("Failed to open file '%s' for reading.\n", file_name);
            continue;
        }
        defer { fclose(file); };

        char line[BUFSIZ];
        fgets(line, BUFSIZ, file);
        char entity_type_str[4096] = {};
        sscanf(line, "type %s", entity_type_str);

        fgets(line, BUFSIZ, file);
        int id = -1;
        sscanf(line, "id %d", &id);
        
        fgets(line, BUFSIZ, file);
        Vector2 position;
        sscanf(line, "position (%f, %f)", &position.x, &position.y);

        if (strings_match(entity_type_str, "Guy")) {
            Guy *guy = manager->make_guy(id);
            guy->position = position;
            load_guy(guy, file);
        } else if (strings_match(entity_type_str, "Enemy")) {
            Enemy *enemy = manager->make_enemy(id);
            enemy->position = position;
            load_enemy(enemy, file);
        } else if (strings_match(entity_type_str, "Thumbleweed")) {
            Thumbleweed *tw = manager->make_thumbleweed(id);
            tw->position = position;
            load_thumbleweed(tw, file);
        } else if (strings_match(entity_type_str, "Light_Source")) {
            Light_Source *ls = manager->make_light_source(id);
            ls->position = position;
            load_light_source(ls, file);
        } else if (strings_match(entity_type_str, "Tree")) {
            Tree *tree = manager->make_tree(id);
            tree->position = position;
            load_tree(tree, file);
        }
    }

    Tilemap *tilemap = manager->make_tilemap();
    load_tilemap(tilemap, tilemap_name);
    tilemap->position = Vector2(-8.0f, -4.5f);
    
    Camera *camera = new Camera();
    camera->position = Vector2(0, 0);
    camera->zoom_t_target = 1.0f;
    camera->zoom_t = 1.0f;
    manager->camera = camera;
    
    return info;
}

// @TODO: Replace FILE * with String_Builder and make os_write_entire_file

static char *entity_type_string(Entity_Type type) {
    switch (type) {
        case ENTITY_TYPE_GUY: return "Guy";
        case ENTITY_TYPE_TILEMAP: return "Tilemap";
        case ENTITY_TYPE_ENEMY: return "Enemy";
        case ENTITY_TYPE_THUMBLEWEED: return "Thumbleweed";
        case ENTITY_TYPE_LIGHT_SOURCE: return "Light_Source";
        case ENTITY_TYPE_TREE: return "Tree";
    }

    return "(unknown)";
}

static void save_entity(FILE *file, Entity *e) {
    fprintf(file, "type %s\n", entity_type_string(e->type));
    fprintf(file, "id %d\n", e->id);

    fprintf(file, "position (%f, %f)\n", e->position.x, e->position.y);
    //fprintf(file, "size (%f, %f)\n", e->size.x, e->size.y);
}

static bool save_guy(char *filepath, Guy *guy) {
    FILE *file = fopen(filepath, "wt");
    if (!file) {
        log_error("Failed to open file '%s' for writing.\n", filepath);
        return false;
    }
    defer { fclose(file); };

    save_entity(file, guy);
    
    fprintf(file, "is_active %d\n", guy->is_active ? 1 : 0);
    fprintf(file, "light_source_id %d\n", guy->light_source_id);
    
    return true;
}

static bool save_enemy(char *filepath, Enemy *enemy) {
    FILE *file = fopen(filepath, "wt");
    if (!file) {
        log_error("Failed to open file '%s' for writing.\n", filepath);
        return false;
    }
    defer { fclose(file); };

    save_entity(file, enemy);
    
    return true;    
}

static bool save_thumbleweed(char *filepath, Thumbleweed *tw) {
    FILE *file = fopen(filepath, "wt");
    if (!file) {
        log_error("Failed to open file '%s' for writing.\n", filepath);
        return false;
    }
    defer { fclose(file); };

    save_entity(file, tw);
    
    return true;
}

static bool save_light_source(char *filepath, Light_Source *ls) {
    FILE *file = fopen(filepath, "wt");
    if (!file) {
        log_error("Failed to open file '%s' for writing.\n", filepath);
        return false;
    }
    defer { fclose(file); };

    save_entity(file, ls);
    
    fprintf(file, "radius %f\n", ls->radius);
    fprintf(file, "color (%f, %f, %f)\n", ls->color.x, ls->color.y, ls->color.z);
    
    return true;
}

static bool save_tree(char *filepath, Tree *tree) {
    FILE *file = fopen(filepath, "wt");
    if (!file) {
        log_error("Failed to open file '%s' for writing.\n", filepath);
        return false;
    }
    defer { fclose(file); };

    save_entity(file, tree);
    
    return true;
}

static bool save_current_game_mode() {
    auto info = globals.current_game_mode;
    auto manager = info->entity_manager;
    
    os_make_directory_if_not_exist("data/saves");

    char *dirpath = tprint("data/saves/%s", info->savegame_name);
    os_make_directory_if_not_exist(dirpath);

    Array <char *> entity_file_paths;
    for (Entity *e : manager->all_entities) {
        if (e->type == ENTITY_TYPE_TILEMAP) continue;
        
        char *entity_file_path = tprint("%s/entity_%d.entity_text", dirpath, e->id);
        entity_file_paths.add(entity_file_path);

        if (e->type == ENTITY_TYPE_GUY) save_guy(entity_file_path, (Guy *)e);
        else if (e->type == ENTITY_TYPE_ENEMY) save_enemy(entity_file_path, (Enemy *)e);
        else if (e->type == ENTITY_TYPE_THUMBLEWEED) save_enemy(entity_file_path, (Thumbleweed *)e);
        else if (e->type == ENTITY_TYPE_LIGHT_SOURCE) save_light_source(entity_file_path, (Light_Source *)e);
        else if (e->type == ENTITY_TYPE_TREE) save_tree(entity_file_path, (Tree *)e);
    }
    
    char *full_path = tprint("%s/%s.level_info", dirpath, info->savegame_name);
    FILE *file = fopen(full_path, "wt");
    if (!file) {
        log_error("Failed to open file '%s' for writing.\n", full_path);
        return false;
    }
    defer { fclose(file); };

    fprintf(file, "[%d] # Version number\n", GAME_MODE_FILE_VERSION);

    fprintf(file, "tilemap %s\n", manager->tilemap ? manager->tilemap->name : "(unknown)");
    
    for (char *fp : entity_file_paths) {
        fprintf(file, "%s\n", fp);
    }

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

    Tree *tree0 = manager->make_tree();
    tree0->size.y = 2.0f;
    tree0->size.x = tree0->size.y * 0.833333f;
    tree0->position.x = -6.0f - (tree0->size.x * 0.5f);
    tree0->position.y = +1.25f + (tree0->size.y * 0.5f);
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
