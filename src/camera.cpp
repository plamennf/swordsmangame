#include "pch.h"
#include "camera.h"
#include "game.h"

void Camera::handle_zoom(int delta) {
    zoom_level -= delta * 0.001f;
    zoom_level = Max(zoom_level, 0.01f);
}

void Camera::update(float dt) {
    if (is_key_down(KEY_SPACE) && is_key_down(MOUSE_BUTTON_LEFT)) {
        float speed = 2.0f;
        
        float x_delta = (float)globals.mouse_x_offset;
        float y_delta = (float)globals.mouse_y_offset;
        
        position.x += x_delta * speed * dt;
        position.y += y_delta * speed * dt;
        
        globals.camera_is_moving = true;
    } else {
        globals.camera_is_moving = false;
    }
}

Matrix4 Camera::get_matrix() {
    Matrix4 result;
    result.identity();

    result._14 = -position.x;
    result._24 = -position.y;
    
    return result;
}
