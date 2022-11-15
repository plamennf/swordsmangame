#include "pch.h"
#include "animation.h"
#include "game.h"
#include "texture_registry.h"
#include "text_file_handler.h"

#define ANIMATION_FILE_VERSION 1

void Animation::update(float dt) {
    accumulated_dt += dt;
    if (accumulated_dt >= inv_sampler_rate) {
        frame_index++;
        if (frame_index >= num_frames) {
            if (is_looping) {
                frame_index = 0;
            } else {
                frame_index = num_frames-1;
                is_completed = true;
            }
        }
        accumulated_dt -= inv_sampler_rate;
    }
}

Texture *Animation::get_current_frame() {
    if (!frames) return NULL;

    assert(frame_index >= 0);
    assert(frame_index < num_frames);
    return frames[frame_index];
}

void Animation::reset() {
    accumulated_dt = 0.0f;
    is_completed = false;
    frame_index = 0;
}

bool load_animation(Animation *animation, char *filepath) {
    Text_File_Handler handler;
    handler.start_file(filepath, filepath, tprint("load_animation:%s", filepath));

    if (handler.version > ANIMATION_FILE_VERSION) {
        handler.report_error("Version is too high");
        return false;
    }

    char *line = handler.consume_next_line();
    if (!starts_with(line, "sampler_rate")) {
        handler.report_error("Expected sampler_rate, but found: %s", line);
        return false;
    }
    line += 12;
    line = eat_spaces(line);
    line = eat_trailing_spaces(line);

    int sampler_rate = atoi(line);
    if (sampler_rate < 1) sampler_rate = 1;
    float inv_sampler_rate = 1.0f / (float)sampler_rate;

    line = handler.consume_next_line();    
    if (!starts_with(line, "is_looping")) {
        handler.report_error("Expected is_looping, but found: %s", line);
        return false;
    }
    line += 10;
    line = eat_spaces(line);
    line = eat_trailing_spaces(line);

    bool is_looping = true;
    if (strings_match(line, "true") || strings_match(line, "True") || strings_match(line, "1")) {
        is_looping = true;
    } else if (strings_match(line, "false") || strings_match(line, "False") || strings_match(line, "0")) {
        is_looping = false;
    } else {
        handler.report_error("Invalid value for is_looping: %s", line);
        handler.report_error("Valid values are");
        handler.report_error("    true");
        handler.report_error("    false");
        handler.report_error("    1");
        handler.report_error("    0");
        return false;
    }
    
    line = handler.consume_next_line();    
    if (!starts_with(line, "num_frames")) {
        handler.report_error("Expected num_frames, but found: %s", line);
        return false;
    }
    line += 10;
    line = eat_spaces(line);
    line = eat_trailing_spaces(line);

    int num_frames = atoi(line);

    if (animation->frames) {
        delete [] animation->frames;
        animation->frames;
    }
    animation->frames = new Texture*[num_frames];
    for (int i = 0; i < num_frames; i++) {
        char *line = handler.consume_next_line();
        if (!line) {
            handler.report_error("Expected %d frames", num_frames);
            delete animation->frames;
            return false;
        }
        animation->frames[i] = globals.texture_registry->get(line);
    }
    
    animation->num_frames = num_frames;
    animation->frame_index = 0;
    animation->inv_sampler_rate = inv_sampler_rate;
    animation->is_looping = is_looping;
    
    return true;
}
