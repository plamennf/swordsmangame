#pragma once

#include "array.h"
#include "hash_table.h"

struct Entity;
struct Guy;
struct Tilemap;
struct Enemy;
struct Thumbleweed;
struct Light_Source;
struct Tree;

struct Camera;

struct Entities_By_Type {
    Array <Guy *> _Guy;
    Array <Enemy *> _Enemy;
    Array <Thumbleweed *> _Thumbleweed;
    Array <Light_Source *> _Light_Source;
    Array <Tree *> _Tree;
};

struct Entity_Manager {
    Entities_By_Type by_type;
    Hash_Table <int, Entity *> entity_lookup;
    Array <Entity *> all_entities;
    int next_entity_id = 0;

    Camera *camera = NULL;
    Tilemap *tilemap = NULL;

    Entity *get_entity_by_id(int id);
    Entity *add_entity(Entity *e);
    
    Guy *make_guy();
    Tilemap *make_tilemap();
    Enemy *make_enemy();
    Thumbleweed *make_thumbleweed();
    Tree *make_tree();
    
    Light_Source *make_light_source();
    
    Guy *get_active_hero();
    void set_active_hero(Guy *guy);
    
private:
    void register_entity(Entity *e);
};
