#include "pch.h"
#include "animation_registry.h"
#include "animation.h"
#include "os.h"

Animation *Animation_Registry::get(char *name) {
    Animation **_animation = animation_lookup.find(name);
    if (_animation) return *_animation;

    char *full_path = tprint("%s/%s.anim", ANIMATIONS_DIRECTORY, name);
    if (!file_exists(full_path)) {
        log_error("Unable to find file '%s.anim' in '%s'.\n", name, ANIMATIONS_DIRECTORY);
        return NULL;
    }

    Animation *animation = new Animation();
    bool success = load_animation(animation, full_path);
    if (!success) {
        log_error("Unable to load animation '%s'.\n", full_path);
        delete animation;
        return NULL;
    }
    
    u64 modtime = 0;
    get_file_last_write_time(full_path, &modtime);
    
    animation->full_path = copy_string(full_path);
    animation->name = copy_string(name);
    animation->modtime = modtime;
    
    animation_lookup.add(name, animation);
    loaded_animations.add(animation);
    
    return animation;
}

void Animation_Registry::do_hotloading() {
    for (int i = 0; i < loaded_animations.count; i++) {
        Animation *animation = loaded_animations[i];

        u64 modtime = animation->modtime;
        bool success = get_file_last_write_time(animation->full_path, &modtime);
        if (!success) continue;

        if (animation->modtime != modtime) {
            animation->modtime = modtime;
            load_animation(animation, animation->full_path);
        }
    }
}
