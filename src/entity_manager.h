#pragma once

#include "array.h"
#include "hash_table.h"

struct Entity;
struct Guy;
struct Tilemap;

struct Entities_By_Type {
    Array <Guy *> _Guy;
    Array <Tilemap *> _Tilemap;
};

struct Entity_Manager {
    Entities_By_Type by_type;
    Hash_Table <int, Entity *> entity_lookup;
    int next_entity_id = 0;
    
    Guy *make_guy();
    Tilemap *make_tilemap();

private:
    void register_entity(Entity *e);
};
