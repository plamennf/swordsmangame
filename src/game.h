#pragma once

#include "event.h"
#include "array.h"
#include "geometry.h"

#define WORLD_SPACE_SIZE_X 16.0f
#define WORLD_SPACE_SIZE_Y 9.0f

struct Shader_Registry;
struct Texture_Registry;
struct Animation_Registry;

struct Shader;
struct Animation;

struct Entity_Manager;

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

    int font_page_size_x = 0;
    int font_page_size_y = 0;
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

Vector2 world_space_to_screen_space(Vector2 v);

Entity_Manager *get_entity_manager();
