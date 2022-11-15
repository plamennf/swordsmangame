#include "pch.h"
#include "entities.h"
#include "entity_manager.h"
#include "text_file_handler.h"
#include "os.h"
#include "game.h"
#include "texture_registry.h"
#include "animation.h"

bool load_tilemap(Tilemap *tilemap, char *name) {
    s64 mark = get_temporary_storage_mark();
    defer { set_temporary_storage_mark(mark); };
    
    char *full_path = tprint("data/tilemaps/%s.tm", name);
    if (!file_exists(full_path)) {
        return false;
    }
    
    Text_File_Handler handler;
    handler.start_file(name, full_path, "load_tilemap");
    if (handler.failed) {
        return false;
    }
    
    // Parse width
    char *line = handler.consume_next_line();
    if (!line) {
        handler.report_error("File '%s' is too short to be considered a valid .tm file.\n", name);
        return false;
    }
    if (!starts_with(line, "width")) {
        handler.report_error("width is missing.\n");
        return false;
    }
    line += 5;
    line = eat_spaces(line);
    line = eat_trailing_spaces(line);
    int width = atoi(line);
    if (width <= 0) {
        handler.report_error("width should be greater than 0.\n");
        return false;
    }

    // Parse height
    line = handler.consume_next_line();
    if (!line) {
        handler.report_error("File '%s' is too short to be considered a valid .tm file.\n", name);
        return false;
    }
    if (!starts_with(line, "height")) {
        handler.report_error("height is missing.\n");
        return false;
    }
    line += 6;
    line = eat_spaces(line);
    line = eat_trailing_spaces(line);
    int height = atoi(line);
    if (height <= 0) {
        handler.report_error("height should be greater than 0.\n");
        return false;
    }

    line = handler.consume_next_line();
    if (!line) {
        handler.report_error("File '%s' is too short to be considered a valid .tm file.\n", name);
        return false;
    }
    if (!starts_with(line, "num_textures")) {
        handler.report_error("num_textures is missing.\n");
        return false;
    }
    line += 12;
    line = eat_spaces(line);
    line = eat_trailing_spaces(line);
    int num_textures = atoi(line);
    if (num_textures <= 0) {
        handler.report_error("num_textures should be greater than 0.\n");
        return false;
    }
    
    Texture **textures = new Texture*[num_textures];
    int current_texture_index = 0;

    for (int i = 0; i < num_textures; i++) {
        line = handler.consume_next_line();
        if (!line) {
            handler.report_error("File '%s' is too short to be considered a valid .tm file.\n", name);
            return false;
        }
        if (!starts_with(line, "texture")) {
            handler.report_error("texture is missing.\n", name);
            return false;
        }
        line += 7;
        line = eat_spaces(line);
        line = eat_trailing_spaces(line);
        
        textures[current_texture_index] = globals.texture_registry->get(line);
        current_texture_index++;
    }

    Tile *tiles = new Tile[width * height];
    
    Array <unsigned char> collidable_ids;
    collidable_ids.use_temporary_storage = true;
    line = handler.consume_next_line();
    if (!starts_with(line, "collidable_ids")) {
        handler.report_error("collidable_ids is missing.\n");
        delete [] tiles;
        return false;
    }
    line += 14;
    line = eat_spaces(line);
    {
        char *at = line;
        while (*at && *at != '\n') {
            char tile_string[16] = {}; // Max is 255.
            for (int i = 0; *at != ','; i++, at++) {
                tile_string[i] = *at;
            }
            at++;
            collidable_ids.add((unsigned char)atoi(tile_string));
        }
    }

    char **lines = new char *[height];
    defer { delete [] lines; };

    for (int i = 0; i < height; i++) {
        lines[i] = handler.consume_next_line();
    }

    for (int y = height-1, i = 0; y >= 0; y--, i++) {
        line = lines[y];
        char *at = line;
        for (int x = 0; x < width; x++) {
            char tile_string[16] = {}; // Max is 255.
            for (int i = 0; *at != ','; i++, at++) {
                tile_string[i] = *at;
            }
            at++;

            unsigned char tile_id = (unsigned char)atoi(tile_string);

            Tile *tile = &tiles[i * width + x];
            tile->id = tile_id;
            tile->is_collidable = collidable_ids.find(tile_id) != -1;
        }
    }

    tilemap->full_path = copy_string(full_path);
    tilemap->name = copy_string(name);

    tilemap->width = width;
    tilemap->height = height;
    tilemap->tiles = tiles;

    tilemap->num_textures = num_textures;
    tilemap->textures = textures;
    
    return true;
}

void Guy::set_animation(Animation *animation) {
    current_animation = animation;
    current_animation->reset();
}

void Guy::set_state(Guy_State state) {
    if (current_state == state) return;
    current_state = state;
    update_animation();
}

void Guy::set_orientation(Guy_Orientation ori) {
    if (orientation == ori) return;
    orientation = ori;
    update_animation();
}

void Guy::update_animation() {
    Animation *animation = looking_down_idle_animation;
    if (current_state == GUY_STATE_IDLE) {
        if (orientation == GUY_LOOKING_DOWN) {
            animation = looking_down_idle_animation;
        } else if (orientation == GUY_LOOKING_RIGHT) {
            animation = looking_right_idle_animation;
        } else if (orientation == GUY_LOOKING_UP) {
            animation = looking_up_idle_animation;
        } else if (orientation == GUY_LOOKING_LEFT) {
            animation = looking_left_idle_animation;
        }
    } else if (current_state == GUY_STATE_MOVING) {
        if (orientation == GUY_LOOKING_DOWN) {
            animation = looking_down_moving_animation;
        } else if (orientation == GUY_LOOKING_RIGHT) {
            animation = looking_right_moving_animation;
        } else if (orientation == GUY_LOOKING_UP) {
            animation = looking_up_moving_animation;
        } else if (orientation == GUY_LOOKING_LEFT) {
            animation = looking_left_moving_animation;
        }        
    }
    
    set_animation(animation);    
}
