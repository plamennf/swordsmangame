#pragma once

struct Texture;

struct Animation {
    char *full_path = NULL;
    char *name = NULL;
    u64 modtime = 0;
    
    int num_frames = 0;
    Texture **frames = NULL;
    int frame_index = 0;
    
    float inv_sampler_rate = 0.0f;
    bool is_looping = true;
    bool is_completed = false; // Only valid if is_looping is false
    
    float accumulated_dt = 0.0f;
    
    void update(float dt);
    Texture *get_current_frame();
    void reset();
};

bool load_animation(Animation *animation, char *filepath);
