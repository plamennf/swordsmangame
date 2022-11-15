#pragma once

#include "hash_table.h"
#include "array.h"

#define ANIMATIONS_DIRECTORY "data/animations"

struct Animation;

struct Animation_Registry {
    String_Hash_Table <Animation *> animation_lookup;
    Array <Animation *> loaded_animations;
    
    Animation *get(char *name);
    void do_hotloading();
};
