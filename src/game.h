#pragma once

#include "event.h"
#include "array.h"
#include "geometry.h"

#define WORLD_SPACE_SIZE_X 16.0f
#define WORLD_SPACE_SIZE_Y 9.0f

struct Shader_Registry;
struct Texture_Registry;
struct Animation_Registry;
struct Variable_Service;

struct Shader;
struct Animation;

struct Entity_Manager;
struct Keymap;

#define GAME_MODE_FILE_VERSION 1

enum Game_Mode {
    GAME_MODE_OVERWORLD,
};

struct Game_Mode_Info {
    char *savegame_name;
    Game_Mode game_mode;
    Entity_Manager *entity_manager;
};

struct Time_Info {
    float current_time = 0.0f;
    float current_dt = 0.0f;

    float ui_time = 0.0f;
    float ui_dt = 0.0f;

    float real_world_time = 0.0f;
    float real_world_dt = 0.0f;
};

enum Program_Mode {
    PROGRAM_MODE_GAME,
    PROGRAM_MODE_MENU,
    PROGRAM_MODE_EDITOR,
};

enum Render_Type {
    RENDER_TYPE_LIGHTS,
    RENDER_TYPE_MAIN,
};

struct Game_Globals {
    double last_time = 0.0;
    double time_rate = 1.0;
    Time_Info time_info;

    Game_Mode_Info *current_game_mode;
    
    bool should_quit_game = false;

    Array <Event> events_this_frame;
    Array <Window_Resize_Record> window_resizes;
    
    int display_width = 0;
    int display_height = 0;
    Window_Type my_window = NULL;

    int world_space_size_x = 0;
    int world_space_size_y = 0;
    
    Rectangle2i render_area;
    int render_width = 0;
    int render_height = 0;

    Shader_Registry *shader_registry = NULL;
    Texture_Registry *texture_registry = NULL;
    Animation_Registry *animation_registry = NULL;

    Shader *shader_color = NULL;
    Shader *shader_texture = NULL;
    Shader *shader_text = NULL;
    Shader *shader_guy = NULL;
    Shader *shader_tile = NULL;
    Shader *shader_lightmap_fx = NULL;
    Shader *shader_light = NULL;
    Shader *shader_shadow_segments = NULL;
    Shader *shader_alpha_clear = NULL;

    int font_page_size_x = 0;
    int font_page_size_y = 0;

    Program_Mode program_mode = PROGRAM_MODE_GAME;
    Render_Type render_type = RENDER_TYPE_MAIN;
    
    int mouse_x_offset = 0;
    int mouse_y_offset = 0;

    bool draw_cursor = false;
    bool camera_is_moving = false;
    bool app_is_focused = true;

    float zoom_speed = 0.01f;
    
    Keymap *keymap = NULL;
    Variable_Service *variable_service = NULL;

    Game_Globals();
};

extern Game_Globals globals;

float get_gameplay_dt();

struct Key_Info {
    bool is_down;
    bool was_down;
    bool changed;
};

bool is_key_down(int key_code);
bool is_key_pressed(int key_code);
bool was_key_just_released(int key_code);

Game_Mode_Info *load_game_mode(Game_Mode game_mode);

Entity_Manager *get_entity_manager();

struct Key_Action;

bool is_key_down(Key_Action action);
bool is_key_pressed(Key_Action action);
bool was_key_just_released(Key_Action action);

void set_matrix_for_entities(Entity_Manager *manager);
void draw_main_scene(Entity_Manager *manager);
void resolve_to_screen();

void toggle_editor();

float move_toward(float a, float b, float amount);

Vector2 screen_space_to_world_space(int x, int y, bool is_position);
