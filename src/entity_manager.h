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
    Array <Tilemap *> _Tilemap;
    Array <Enemy *> _Enemy;
};

struct Entity_Manager {
    Entities_By_Type by_type;
    Hash_Table <int, Entity *> entity_lookup;
    int next_entity_id = 0;

    Camera *camera = NULL;
    
    Guy *make_guy();
    Tilemap *make_tilemap();
    Enemy *make_enemy();
    
private:
    void register_entity(Entity *e);
};
