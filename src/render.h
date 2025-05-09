#pragma once

#include "geometry.h"
#include "shader.h"
#include "texture.h"

struct Color_Target {
    Texture *texture;
    ID3D11RenderTargetView *rtv;
};

struct Depth_Target {
    Texture *texture;
    ID3D11DepthStencilView *dsv;
};

extern Color_Target *the_back_buffer;

extern Color_Target *the_offscreen_buffer;
extern Color_Target *the_lightmap_buffer;

void init_render(Window_Type window_handle, int width, int height, bool vsync);
void swap_buffers();

Color_Target *create_color_target(int width, int height);
void release_color_target(Color_Target *ct);

void render_resize(int width, int height);

void set_render_targets(Color_Target *ct, Depth_Target *dt);
void clear_color_target(Color_Target *ct, float r, float g, float b, float a, Rectangle2i *rect = NULL);
void clear_depth_target(Depth_Target *dt, float z);

void set_viewport(int x, int y, int width, int height);
void set_scissor(int x, int y, int width, int height);

void immediate_begin();
void immediate_flush();
void immediate_quad(Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3, Vector4 color);
void immediate_quad(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, Vector4 color);
void immediate_quad(float x0, float y0, float x1, float y1, Vector4 color);
void immediate_quad(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, Vector2 uv0, Vector2 uv1, Vector2 uv2, Vector2 uv3, Vector4 color);
void immediate_quad(float x0, float y0, float x1, float y1, Vector2 uv0, Vector2 uv1, Vector2 uv2, Vector2 uv3, Vector4 color);
void immediate_triangle(Vector2 p0, Vector2 p1, Vector2 p2, Vector4 color);

Shader *set_shader(Shader *shader);
