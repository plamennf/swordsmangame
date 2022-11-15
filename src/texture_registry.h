#pragma once

#include "hash_table.h"
#include "array.h"

#define TEXTURE_DIRECTORY "data/textures"

struct Texture;

struct Texture_Registry {
    String_Hash_Table <Texture *> texture_lookup;
    Array <Texture *> loaded_textures;
    
    Texture *get(char *name);
    void do_hotloading();
};
