#pragma once

#include "geometry.h"

struct Camera {
    Vector2 position = Vector2(0, 0);
    float zoom_level = 1.0f;

    void handle_zoom(int delta);
    void update(float dt);
    Matrix4 get_matrix();
};
