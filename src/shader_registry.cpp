#include "pch.h"
#include "shader_registry.h"
#include "os.h"
#include "shader.h"

extern bool load_shader(Shader *shader, char *filepath);

Shader *Shader_Registry::get(char *name) {
    Shader **_shader = shader_lookup.find(name);
    if (_shader) return *_shader;

    char *full_path = tprint("%s/%s.hlsl", SHADER_DIRECTORY, name);
    if (!file_exists(full_path)) {
        log_error("Unable to find file '%s.hlsl' in '%s'.\n", name, SHADER_DIRECTORY);
        return NULL;
    }
    
    Shader *shader = new Shader();
    bool success = load_shader(shader, full_path);
    if (!success) {
        log_error("Unable to load shader '%s'.\n", full_path);
        delete shader;
        return NULL;
    }

    u64 modtime = 0;
    get_file_last_write_time(full_path, &modtime);

    shader->full_path = copy_string(full_path);
    shader->name = copy_string(name);
    shader->modtime = modtime;
    
    shader_lookup.add(name, shader);
    loaded_shaders.add(shader);
    
    return shader;
}

void Shader_Registry::do_hotloading() {
    for (int i = 0; i < loaded_shaders.count; i++) {
        Shader *shader = loaded_shaders[i];

        u64 modtime = shader->modtime;
        bool success = get_file_last_write_time(shader->full_path, &modtime);
        if (!success) continue;

        if (shader->modtime != modtime) {
            shader->modtime = modtime;
            load_shader(shader, shader->full_path);
        }
    }
}
