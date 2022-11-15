#include "pch.h"
#include "texture_registry.h"
#include "os.h"
#include "texture.h"

Texture *Texture_Registry::get(char *name) {
    Texture **_texture = texture_lookup.find(name);
    if (_texture) return *_texture;

    char *extensions[] = {
        "png",
        "jpg",
        "bmp",
    };
    
    char *full_path = NULL;//tprint("%s/%s.hlsl", TEXTURE_DIRECTORY, name);
    for (int i = 0; i < ArrayCount(extensions); i++) {
        full_path = tprint("%s/%s.%s", TEXTURE_DIRECTORY, name, extensions[i]);
        if (file_exists(full_path)) {
            break;
        } else {
            full_path = NULL;
        }
    }
    
    if (!full_path) {
        log_error("Unable to find file '%s' in '%s'.\n", name, TEXTURE_DIRECTORY);
        return NULL;
    }
    
    Texture *texture = new Texture();
    bool success = load_texture_from_file(texture, full_path);
    if (!success) {
        log_error("Unable to load texture '%s'.\n", name);
        delete texture;
        return NULL;
    }

    u64 modtime = 0;
    get_file_last_write_time(full_path, &modtime);

    texture->full_path = copy_string(full_path);
    texture->name = copy_string(name);
    texture->modtime = modtime;
    
    texture_lookup.add(name, texture);
    loaded_textures.add(texture);
    
    return texture;
}

void Texture_Registry::do_hotloading() {
    for (int i = 0; i < loaded_textures.count; i++) {
        Texture *texture = loaded_textures[i];

        u64 modtime = texture->modtime;
        bool success = get_file_last_write_time(texture->full_path, &modtime);
        if (!success) continue;

        if (texture->modtime != modtime) {
            texture->modtime = modtime;
            load_texture_from_file(texture, texture->full_path);
        }
    }
}
