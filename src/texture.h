#pragma once

enum Texture_Format {
    TEXTURE_FORMAT_UNKNOWN,
    TEXTURE_FORMAT_RGBA8,
    TEXTURE_FORMAT_R8,
};

struct Bitmap {
    int width = 0;
    int height = 0;
    int bytes_per_pixel = 0;
    Texture_Format format = TEXTURE_FORMAT_UNKNOWN;
    u8 *data = NULL;
};

struct Texture {
    char *full_path = NULL;
    char *name = NULL;
    u64 modtime = 0;
    
    int width = 0;
    int height = 0;

    Texture_Format format = TEXTURE_FORMAT_UNKNOWN;
    int bytes_per_pixel = 0;
    
    ID3D11Texture2D *texture = NULL;
    ID3D11ShaderResourceView *srv = NULL;
};

bool load_bitmap(Bitmap *bitmap, char *filepath);
void deinit(Bitmap *bitmap);

bool load_texture_from_bitmap(Texture *texture, Bitmap *bitmap);
bool load_texture_from_file(Texture *texture, char *filepath);

void set_texture(int slot, Texture *texture);
void update_texture(Texture *texture, int x, int y, int width, int height, u8 *data);
