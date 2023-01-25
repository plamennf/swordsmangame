#pragma once

struct Dynamic_Font;
struct Entity_Manager;

void rendering_2d_right_handed(int width, int height);
void draw_text(Dynamic_Font *font, char *text, int x, int y, Vector4 color);

void set_matrix_for_entities(Entity_Manager *manager);
void draw_main_scene(Entity_Manager *manager);
void resolve_to_screen();

void draw_circle(Vector2 center, float radius, Vector4 color);

void draw_lights();
void draw_one_frame();
