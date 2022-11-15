#pragma once

#include "geometry.h"

struct Dynamic_Font;

void rendering_2d_right_handed(int width, int height);
void draw_text(Dynamic_Font *font, char *text, int x, int y, Vector4 color);
