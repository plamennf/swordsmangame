#pragma once

#include "geometry.h"

enum Render_Vertex_Type {
    RENDER_VERTEX_XCUN,
};

struct Vertex_XCUN {
    Vector3 position;
    Vector4 color;
    Vector2 uv;
    Vector3 normal;
};
