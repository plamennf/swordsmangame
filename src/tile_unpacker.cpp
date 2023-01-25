#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

static void output_images(char *image_filepath, unsigned char *image_data, int image_width, int image_height, int tile_width, int tile_height) {
    int tile_counter = 0;
    for (int y = 0; y < image_height; y += tile_height) {
        for (int x = 0; x < image_width; x += tile_width) {
            //unsigned char *at = image_data + ((y * image_width + x) * 4);

            unsigned char *output_data = new unsigned char[tile_width * tile_height * 4];
            for (int iy = 0; iy < tile_height; iy++) {
                for (int ix = 0; ix < tile_width; ix++) {
                    unsigned char *source = &image_data[((y + iy) * image_width + (x + ix)) * 4];
                    unsigned char *dest = &output_data[(iy * tile_width + ix) * 4];

                    dest[0] = source[0];
                    dest[1] = source[1];
                    dest[2] = source[2];
                    dest[3] = source[3];
                }
            }
            
            char filepath[4096] = {};
            snprintf(filepath, sizeof(filepath), "%s_%d.png", image_filepath, tile_counter);
            
            stbi_write_png(filepath, tile_width, tile_height, 4, output_data, tile_width * 4);

            free(output_data);
            tile_counter++;
        }
    }
}

static void print_usage() {
    fprintf(stderr, "tile_unpacker.exe <filename> <tile width> <tile height>\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "No input filepath provided.\n");
        print_usage();
        return 1;
    }

    char *filepath = _strdup(argv[1]);
    
    if (argc < 4) {
        fprintf(stderr, "No tile size provided.\n");
        print_usage();
        return 1;
    }

    int tile_width = atoi(argv[2]);
    int tile_height = atoi(argv[3]);

    int image_width, image_height, image_channels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char *data = stbi_load(filepath, &image_width, &image_height, &image_channels, 4);
    if (!data) {
        fprintf(stderr, "Failed to load file '%s'.\n", filepath);
        return 1;
    }
    
    char *dot = strrchr(filepath, '.');
    if (dot) {
        filepath[dot - filepath] = 0;
    }
    
    output_images(filepath, data, image_width, image_height, tile_width, tile_height);

    stbi_image_free(data);
    free(filepath);
    return 0;
}
