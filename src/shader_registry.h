#pragma once

#include "hash_table.h"
#include "array.h"

#define SHADER_DIRECTORY "data/shaders"

struct Shader;

// TODO: Load shaders on init
struct Shader_Registry {
    String_Hash_Table <Shader *> shader_lookup;
    Array <Shader *> loaded_shaders;
    
    Shader *get(char *name);
    void do_hotloading();
};
