#pragma once

#include "geometry.h"

struct Entity_Manager;
struct Texture;

struct Animation;

// Do not move these around otherwise savegames won't work.
enum Entity_Type {
    ENTITY_TYPE_GUY = 0,
    ENTITY_TYPE_TILEMAP = 1,
    ENTITY_TYPE_ENEMY = 2,
    ENTITY_TYPE_THUMBLEWEED = 3,
    ENTITY_TYPE_LIGHT_SOURCE = 4,
};

enum Entity_Flags {
    EF_CAN_CAST_SHADOWS = 1,
};

struct Entity {
    Entity_Type type;
    int id;
    Entity_Manager *manager;
    int flags;

    Vector2 position;
    Vector2 size;

    Animation *current_animation;

    void update_current_animation(float dt);
};

enum Guy_State {
    GUY_STATE_IDLE,
    GUY_STATE_MOVING,
};

enum Guy_Orientation {
    GUY_LOOKING_DOWN,
    GUY_LOOKING_RIGHT,
    GUY_LOOKING_UP,
    GUY_LOOKING_LEFT,
};

struct Guy : public Entity {
    bool is_active = false;
    
    Vector2 velocity;
    Vector2 max_velocity;

    Guy_State current_state = GUY_STATE_IDLE;
    Guy_Orientation orientation = GUY_LOOKING_DOWN;
    
    Animation *looking_down_idle_animation;
    Animation *looking_right_idle_animation;
    Animation *looking_up_idle_animation;
    Animation *looking_left_idle_animation;

    Animation *looking_down_moving_animation;
    Animation *looking_right_moving_animation;
    Animation *looking_up_moving_animation;
    Animation *looking_left_moving_animation;

    int light_source_id;
    
    void set_animation(Animation *animation);
    void set_state(Guy_State state);
    void set_orientation(Guy_Orientation orientation);
    
    void update_animation();
};

struct Tile {
    unsigned char id;
    bool is_collidable;
};

struct Tilemap : public Entity {
    static const int VERSION = 1;

    char *full_path = 0;
    char *name = 0;
    
    int width = 0;
    int height = 0;
    Tile *tiles = 0;
    
    int num_textures = 0;
    Texture **textures = 0;
};

bool load_tilemap(Tilemap *tilemap, char *name);

// @Rename to something else
struct Enemy : public Entity {
    Texture *texture;
};

struct Thumbleweed : public Enemy {
    Animation *idle_animation;
    Animation *moving_animation;
    Animation *attack_animation;
    Animation *transformation_animation;
};

struct Light_Source : public Entity {
    float radius;
    Vector3 color;
};
