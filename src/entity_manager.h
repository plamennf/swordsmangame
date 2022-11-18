#pragma once

#include "array.h"
#include "hash_table.h"

struct Entity;
struct Guy;
struct Tilemap;
struct Enemy;

struct Camera;

struct Entities_By_Type {
    Array <Guy *> _Guy;
    Array <Enemy *> _Enemy;
};

struct Entity_Manager {
    Entities_By_Type by_type;
    Hash_Table <int, Entity *> entity_lookup;
    int next_entity_id = 0;

    Camera *camera = NULL;
    Tilemap *tilemap = NULL;
    
    Guy *make_guy();
    Tilemap *make_tilemap();
    Enemy *make_enemy();

    Guy *get_active_hero();
    void set_active_hero(Guy *guy);
    
private:
    void register_entity(Entity *e);
};
