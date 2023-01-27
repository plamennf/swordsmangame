#include "pch.h"
#include "draw.h"
#include "render.h"
#include "font.h"
#include "entities.h"
#include "entity_manager.h"
#include "game.h"
#include "camera.h"
#include "animation.h"
#include "hud.h"

#include "texture_registry.h"

void rendering_2d_right_handed(int width, int height) {
    Matrix4 m;
    m.identity();

    float w = (float)width;
    if (w < 1.0f) w = 1.0f;
    float h = (float)height;
    if (h < 1.0f) h = 1.0f;
    
    m._11 = 2.0f / w;
    m._22 = 2.0f / h;
    m._14 = -1.0f;
    m._24 = -1.0f;
    
    global_parameters.proj_matrix = m;
    global_parameters.view_matrix.identity();
    global_parameters.transform = global_parameters.proj_matrix * global_parameters.view_matrix;    
}

void draw_text(Dynamic_Font *font, char *text, int x, int y, Vector4 color) {
    Texture *last_texture = NULL;
    
    font->prep_text(text, x, y);
    immediate_begin();
    for (Font_Quad quad : font->font_quads) {
        Vector2 p0(quad.x0, quad.y0);
        Vector2 p1(quad.x1, quad.y0);
        Vector2 p2(quad.x1, quad.y1);
        Vector2 p3(quad.x0, quad.y1);
        
        Vector2 uv0(quad.u0, quad.v1);
        Vector2 uv1(quad.u1, quad.v1);
        Vector2 uv2(quad.u1, quad.v0);
        Vector2 uv3(quad.u0, quad.v0);

        if (last_texture != quad.texture) {
            set_texture(0, quad.texture);
            last_texture = quad.texture;
        }

        immediate_quad(p0, p1, p2, p3, uv0, uv1, uv2, uv3, color);
    }
    immediate_flush();

    font->font_quads.count = 0;
}

void set_matrix_for_entities(Entity_Manager *manager) {
    Tilemap *tm = manager->tilemap;
    Camera *camera = manager->camera;
    
    float half_width = 0.5f * tm->width;
    float half_height = 0.5f  * tm->height;
    
    global_parameters.proj_matrix = make_orthographic(-half_width * camera->zoom_t, half_width * camera->zoom_t, -half_height * camera->zoom_t, half_height * camera->zoom_t);
    global_parameters.view_matrix = camera->get_matrix();
    global_parameters.transform = global_parameters.proj_matrix * global_parameters.view_matrix;    
}

static Shader *get_shader_for_entity(Entity *e) {
    switch (e->type) {
        case ENTITY_TYPE_TILEMAP: return globals.shader_tile;
            
        case ENTITY_TYPE_TREE:
        case ENTITY_TYPE_GUY:
        case ENTITY_TYPE_ENEMY:
        case ENTITY_TYPE_THUMBLEWEED:
            return globals.shader_guy;
    }

    return NULL;
}

static void draw_tilemap(Tilemap *tm) {
    immediate_begin();
    float xpos = tm->position.x;
    float ypos = tm->position.y;
    
    Texture *last_texture = NULL;
    
    for (int y = 0; y < tm->height; y++) {
        for (int x = 0; x < tm->width; x++) {
            Tile *tile = &tm->tiles[y * tm->width + x];
            if (tile->id) {
                Texture *texture = tm->textures[tile->id-1];
                if (texture != last_texture) {
                    immediate_flush();
                    set_texture(0, texture);
                    last_texture = texture;
                }
                        
                Vector2 position(xpos, ypos);
                Vector2 size(1, 1);

                float hw = size.x * 0.5f;
                float hh = size.y * 0.5f;

                Vector2 center(position.x + hw, position.y + hh);
                        
                Vector2 p0 = center + Vector2(-hw, -hh);
                Vector2 p1 = center + Vector2(+hw, -hh);
                Vector2 p2 = center + Vector2(+hw, +hh);
                Vector2 p3 = center + Vector2(-hw, +hh);
                            
                Vector2 uv0(0, 0);
                Vector2 uv1(1, 0);
                Vector2 uv2(1, 1);
                Vector2 uv3(0, 1);
                
                Vector4 color(1, 1, 1, 1);
                
                immediate_quad(p0, p1, p2, p3, uv0, uv1, uv2, uv3, color);
            }
            xpos += 1.0f;
        }
        xpos = tm->position.x;
        ypos += 1.0f;
    }
    immediate_flush();
}

static void draw_entity(Entity *e) {
    auto shader = get_shader_for_entity(e);
    if (!shader) return;
    
    set_shader(shader);
    
    if (e->type == ENTITY_TYPE_TILEMAP) {
        draw_tilemap((Tilemap *)e);
        return;
    }
    
    Animation *animation = e->current_animation;
    if (!animation) return;

    Texture *texture = animation->get_current_frame();
    if (!texture) return;

    set_texture(0, texture);

    Vector2 position = e->position;
    Vector2 size = e->size;
        
    float hw = size.x * 0.5f;
    float hh = size.y * 0.5f;
        
    Vector2 center = position + Vector2(hw, hh);

    Vector2 p0 = center + Vector2(-hw, -hh);
    Vector2 p1 = center + Vector2(+hw, -hh);
    Vector2 p2 = center + Vector2(+hw, +hh);
    Vector2 p3 = center + Vector2(-hw, +hh);

    Vector2 uv0(0, 0);
    Vector2 uv1(1, 0);
    Vector2 uv2(1, 1);
    Vector2 uv3(0, 1);

    Vector4 color(1, 1, 1, 1);
    
    immediate_begin();
    immediate_quad(p0, p1, p2, p3, uv0, uv1, uv2, uv3, color);
    immediate_flush();
}

void draw_main_scene(Entity_Manager *manager) {
    auto tm = manager->tilemap;
    if (tm) draw_entity(tm);

    Array <Tree *> trees_to_draw_after_guy;
    trees_to_draw_after_guy.use_temporary_storage = true;

    Guy *guy = manager->get_active_hero();
    for (Tree *tree : manager->by_type._Tree) {
        if (guy) {
            if (tree->position.y < guy->position.y) {
                trees_to_draw_after_guy.add(tree);
            } else {
                draw_entity(tree);
            }
        } else {
            draw_entity(tree);
        }
    }
    for (Thumbleweed *tw : manager->by_type._Thumbleweed) draw_entity(tw);
    for (Enemy *enemy : manager->by_type._Enemy) draw_entity(enemy);
    for (Guy *guy : manager->by_type._Guy) draw_entity(guy);
    
    for (Tree *tree : trees_to_draw_after_guy) draw_entity(tree);
}

void resolve_to_screen() {
    set_render_targets(the_back_buffer, NULL);
    clear_color_target(the_back_buffer, 0.0f, 0.0f, 0.0f, 1.0f);
    set_viewport(0, 0, globals.display_width, globals.display_height);
    set_scissor(0, 0, globals.display_width, globals.display_height);

    rendering_2d_right_handed(globals.display_width, globals.display_height);
    refresh_global_parameters();
    
    set_shader(globals.shader_lightmap_fx);
    set_texture(0, the_offscreen_buffer->texture);
    set_texture(1, the_lightmap_buffer->texture);
    
    float x0 = (globals.display_width - globals.render_width) * 0.5f;
    float y0 = (globals.display_height - globals.render_height) * 0.5f;
    float x1 = x0 + (float)globals.render_width;
    float y1 = y0 + (float)globals.render_height;

#ifdef RENDER_D3D11
    // Use flipped-y uvs because d3d11 uses left-hand coordinate systems.
    Vector2 uv0(0, 1);
    Vector2 uv1(1, 1);
    Vector2 uv2(1, 0);
    Vector2 uv3(0, 0);
#else
    Vector2 uv0(0, 0);
    Vector2 uv1(1, 0);
    Vector2 uv2(1, 1);
    Vector2 uv3(0, 1);
#endif
    
    Vector4 color(1, 1, 1, 1);
    
    immediate_begin();
    immediate_quad(x0, y0, x1, y1, uv0, uv1, uv2, uv3, color);
    immediate_flush();
}

static Vector4 to_vec4(Vector3 v) {
    return Vector4(v.x, v.y, v.z, 1.0f);
}

void draw_circle(Vector2 center, float radius, Vector4 color) {
    const int NUM_TRIANGLES = 100;
    auto dtheta = TAU / NUM_TRIANGLES;
    float r = radius;

    for (int i = 0; i < NUM_TRIANGLES; i++) {
        auto theta0 = i * dtheta;
        auto theta1 = (i+1) * dtheta;
        
        auto v0 = get_vec2(theta0);
        auto v1 = get_vec2(theta1);

        auto p0 = center;
        auto p1 = center + r * v0;
        auto p2 = center + r * v1;

        immediate_triangle(p0, p1, p2, color);
    }
}

void draw_lights() {
    auto manager = get_entity_manager();
    set_matrix_for_entities(manager);
    refresh_global_parameters();

    immediate_begin();
    for (Light_Source *source : manager->by_type._Light_Source) {        
        set_shader(globals.shader_light);
        draw_circle(source->position, source->radius, to_vec4(source->color));
    }
    immediate_flush();
}

void draw_one_frame() {
    set_render_targets(the_lightmap_buffer, NULL);
    clear_color_target(the_lightmap_buffer, 19/255.0f, 24/255.0f, 98/255.0f, 1.0f);
    set_viewport(0, 0, globals.render_width, globals.render_height);
    set_scissor(0, 0, globals.render_width, globals.render_height);

    globals.render_type = RENDER_TYPE_LIGHTS;
    draw_lights();
    
    set_render_targets(the_offscreen_buffer, NULL);
    clear_color_target(the_offscreen_buffer, 0.0f, 1.0f, 1.0f, 1.0f);
    set_viewport(0, 0, globals.render_width, globals.render_height);
    set_scissor(0, 0, globals.render_width, globals.render_height);
    
    auto manager = get_entity_manager();
    set_matrix_for_entities(manager);
    refresh_global_parameters();

    globals.render_type = RENDER_TYPE_MAIN;
    draw_main_scene(manager);
    
    resolve_to_screen();
    
    draw_hud();
}
