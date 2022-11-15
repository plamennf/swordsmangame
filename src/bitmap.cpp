#include "pch.h"
#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static u8 *copy_4_to_4(u8 *source, int width, int height) {
    u8 *dest = new u8[width * height * 4];
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            u8 *s = &source[(y * width + x) * 4];
            u8 *d = &dest[(y * width + x) * 4];

            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
            d[3] = s[3];
        }
    }
    return dest;
}

static u8 *copy_3_to_4(u8 *source, int width, int height) {
    u8 *dest = new u8[width * height * 4];
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            u8 *s = &source[(y * width + x) * 3];
            u8 *d = &dest[(y * width + x) * 4];

            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
            d[3] = 255;
        }
    }
    return dest;
}

static u8 *copy_2_to_4(u8 *source, int width, int height) {
    u8 *dest = new u8[width * height * 4];
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            u8 *s = &source[(y * width + x) * 2];
            u8 *d = &dest[(y * width + x) * 4];

            d[0] = s[0];
            d[1] = s[1];
            d[2] = 255;
            d[3] = 255;
        }
    }
    return dest;
}

static u8 *copy_1_to_1(u8 *source, int width, int height) {
    u8 *dest = new u8[width * height];
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            dest[y * width + x] = source[y * width + x];
        }
    }
    return dest;
}

bool load_bitmap(Bitmap *bitmap, char *filepath) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    u8 *stb_data = stbi_load(filepath, &width, &height, &channels, 0);
    if (!stb_data) return false;
    defer { stbi_image_free(stb_data); };
    
    u8 *data;
    Texture_Format format;
    int bytes_per_pixel;
    if (channels == 4) {
        data = copy_4_to_4(stb_data, width, height);
        format = TEXTURE_FORMAT_RGBA8;
        bytes_per_pixel = 4;
    } else if (channels == 3) {
        data = copy_3_to_4(stb_data, width, height);
        format = TEXTURE_FORMAT_RGBA8;
        bytes_per_pixel = 4;
    } else if (channels == 2) {
        data = copy_2_to_4(stb_data, width, height);
        format = TEXTURE_FORMAT_RGBA8;
        bytes_per_pixel = 4;
    } else if (channels == 1) {
        data = copy_1_to_1(stb_data, width, height);
        format = TEXTURE_FORMAT_R8;
        bytes_per_pixel = 1;
    } else {
        assert(false);
    }

    bitmap->width = width;
    bitmap->height = height;
    bitmap->bytes_per_pixel = bytes_per_pixel;
    bitmap->format = format;
    bitmap->data = data;
    
    return true;
}

void deinit(Bitmap *bitmap) {
    if (bitmap->data) {
        delete [] bitmap->data;
        bitmap->data = NULL;
    }

    bitmap->width = 0;
    bitmap->height = 0;
    bitmap->bytes_per_pixel = 0;
    bitmap->format = TEXTURE_FORMAT_UNKNOWN;
}
