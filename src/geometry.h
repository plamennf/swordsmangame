#pragma once

#include "general.h"

#include <assert.h>
#include <math.h>

struct Vector2 {
    float x;
    float y;

    Vector2() : x(0.0f), y(0.0f) {}
    Vector2(float x, float y) : x(x), y(y) {}

    inline float &operator[](int index) {
        assert(index >= 0);
        assert(index < 2);
        return *(&x + index);
    }
};

inline Vector2 operator+(Vector2 a, Vector2 b) {
    Vector2 result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

inline Vector2 operator*(Vector2 a, float b) {
    Vector2 result;

    result.x = a.x * b;
    result.y = a.y * b;

    return result;
}

inline Vector2 operator*(float a, Vector2 b) {
    Vector2 result;

    result.x = a * b.x;
    result.y = a * b.y;

    return result;
}

inline Vector2 &operator+=(Vector2 &a, Vector2 b) {
    a.x += b.x;
    a.y += b.y;
    return a;
}

inline Vector2 &operator*=(Vector2 &a, float b) {
    a.x *= b;
    a.y *= b;
    return a;
}

inline float length_squared(Vector2 v) {
    return v.x*v.x + v.y*v.y;
}

inline float length(Vector2 v) {
    return sqrtf(length_squared(v));
}
    
inline Vector2 normalize(Vector2 v) {
    float multiplier = 1.0f / length(v);
    v.x *= multiplier;
    v.y *= multiplier;
    return v;
}

inline Vector2 normalize_or_zero(Vector2 v) {
    Vector2 result(0.0f, 0.0f);
    
    float len_sq = length_squared(v);
    if (len_sq > 0.0001f * 0.0001f) {
        float multiplier = 1.0f / sqrtf(len_sq);
        result.x = v.x * multiplier;
        result.y = v.y * multiplier;
    }

    return result;
}

inline Vector2 componentwise_product(Vector2 a, Vector2 b) {
    Vector2 result;

    result.x = a.x * b.x;
    result.y = a.y * b.y;

    return result;
}

inline Vector2 get_vec2(float theta) {
    float ct = cosf(theta);
    float st = sinf(theta);

    return Vector2(ct, st);
}

struct Vector3 {
    float x;
    float y;
    float z;

    Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vector3(float value) : x(value), y(value), z(value) {}

    inline float &operator[](int index) {
        assert(index >= 0);
        assert(index < 3);
        return *(&x + index);
    }
};

inline Vector3 operator+(Vector3 a, Vector3 b) {
    Vector3 result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

inline Vector3 operator-(Vector3 a, Vector3 b) {
    Vector3 result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return result;
}

inline Vector3 operator*(Vector3 a, float b) {
    Vector3 result;

    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;

    return result;
}

inline Vector3 operator*(float a, Vector3 b) {
    Vector3 result;

    result.x = a * b.x;
    result.y = a * b.y;
    result.z = a * b.z;

    return result;
}

inline Vector3 &operator+=(Vector3 &a, Vector3 b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

inline Vector3 &operator-=(Vector3 &a, Vector3 b) {
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return a;
}

inline bool operator<(Vector3 a, Vector3 b) {
    return a.x < b.x && a.y < b.y && a.z < b.z;
}

inline bool operator>(Vector3 a, Vector3 b) {
    return a.x > b.x && a.y > b.y && a.z > b.z;
}

inline float length_squared(Vector3 v) {
    return v.x*v.x + v.y*v.y + v.z*v.z;
}

inline float length(Vector3 v) {
    return sqrtf(length_squared(v));
}
    
inline Vector3 normalize(Vector3 v) {
    float multiplier = 1.0f / length(v);
    v.x *= multiplier;
    v.y *= multiplier;
    v.z *= multiplier;
    return v;
}

inline Vector3 normalize_or_zero(Vector3 v) {
    Vector3 result(0.0f, 0.0f, 0.0f);
    
    float len_sq = length_squared(v);
    if (len_sq > 0.0001f * 0.0001f) {
        float multiplier = 1.0f / sqrtf(len_sq);
        result.x = v.x * multiplier;
        result.y = v.y * multiplier;
        result.z = v.z * multiplier;
    }

    return result;
}

inline Vector3 cross_product(Vector3 a, Vector3 b) {
    Vector3 result;
    result.x = a.y*b.z - a.z*b.y;
    result.y = a.z*b.x - a.x*b.z;
    result.z = a.x*b.y - a.y*b.x;
    return result;
}

inline float get_barycentric(Vector3 p0, Vector3 p1, Vector3 p2, Vector2 pos) {
    float det = (p1.z - p2.z) * (p0.x - p2.x) + (p2.x - p1.x) * (p0.z - p2.z);
    float l0 = ((p1.z - p2.z) * (pos.x - p2.x) + (p2.x - p1.x) * (pos.y - p2.z)) / det;
    float l1 = ((p2.z - p0.z) * (pos.x - p2.x) + (p0.x - p2.x) * (pos.y - p2.z)) / det;
    float l2 = 1.0f - l0 - l1;
    float result = l0*p0.y + l1*p1.y + l2*p2.y;
    return result;
}

inline Vector3 componentwise_product(Vector3 a, Vector3 b) {
    Vector3 result;

    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;

    return result;
}

inline float distance(Vector3 a, Vector3 b) {
    return sqrtf(Square(a.x-b.x) + Square(a.y-b.y) + Square(a.z-b.z));
}

inline Vector3 lerp(Vector3 a, Vector3 b, float t) {
    return a + t * (b - a);
}

struct Vector4 {
    float x;
    float y;
    float z;
    float w;

    Vector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    inline float &operator[](int index) {
        assert(index >= 0);
        assert(index < 4);
        return *(&x + index);
    }
};

inline Vector4 operator+(Vector4 a, Vector4 b) {
    Vector4 result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;

    return result;
}

inline Vector4 operator*(Vector4 a, float b) {
    Vector4 result;

    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    result.w = a.w * b;

    return result;    
}

inline u32 argb_color(Vector4 color) {
    u32 ir = (u32)(color.x * 255.0f);
    u32 ig = (u32)(color.y * 255.0f);
    u32 ib = (u32)(color.z * 255.0f);
    u32 ia = (u32)(color.w * 255.0f);

    return (ia << 24) | (ir << 16) | (ig << 8) | (ib << 0);
}

struct Vector2i {
    int x;
    int y;

    Vector2i() : x(0), y(0) {}
    Vector2i(int x, int y) : x(x), y(y) {}

    inline int &operator[](int index) {
        assert(index >= 0);
        assert(index < 2);
        return *(&x + index);
    }
};

struct Matrix4 {
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float e[4][4];
    };

    inline void zero() {
        _11 = 0.0f;
        _12 = 0.0f;
        _13 = 0.0f;
        _14 = 0.0f;
        _21 = 0.0f;
        _22 = 0.0f;
        _23 = 0.0f;
        _24 = 0.0f;
        _31 = 0.0f;
        _32 = 0.0f;
        _33 = 0.0f;
        _34 = 0.0f;
        _41 = 0.0f;
        _42 = 0.0f;
        _43 = 0.0f;
        _44 = 0.0f;
    }

    inline void identity() {
        _11 = 1.0f;
        _12 = 0.0f;
        _13 = 0.0f;
        _14 = 0.0f;
        _21 = 0.0f;
        _22 = 1.0f;
        _23 = 0.0f;
        _24 = 0.0f;
        _31 = 0.0f;
        _32 = 0.0f;
        _33 = 1.0f;
        _34 = 0.0f;
        _41 = 0.0f;
        _42 = 0.0f;
        _43 = 0.0f;
        _44 = 1.0f;
    }

    inline void perspective(float aspect_ratio, float fov, float z_near, float z_far) {
        zero();

        float a = 1.0f;
        float b = aspect_ratio;
        float c = fov;

        float n = z_near;
        float f = z_far;

        float d = (n+f) / (n-f);
        float e = (2*f*n) / (n-f);

        _11 = a*c;
        _22 = b*c;
        _33 = d;
        _34 = e;
        _43 = -1.0f;
    }

    inline void x_rotation(float t) {
        identity();
        
        float ct = cosf(t);
        float st = sinf(t);

        _22 = ct;
        _23 = -st;
        _32 = st;
        _33 = ct;
    }

    inline void y_rotation(float t) {
        identity();
        
        float ct = cosf(t);
        float st = sinf(t);

        _11 = ct;
        _31 = -st;
        _13 = st;
        _33 = ct;
    }

    inline void z_rotation(float t) {
        identity();
        
        float ct = cosf(t);
        float st = sinf(t);

        _11 = ct;
        _12 = -st;
        _21 = st;
        _22 = ct;
    }

    inline void look_at(Vector3 pos, Vector3 target, Vector3 world_up) {
        Vector3 z_axis = normalize_or_zero(pos - target);
        Vector3 x_axis = normalize_or_zero(cross_product(normalize_or_zero(world_up), z_axis));
        Vector3 y_axis = cross_product(z_axis, x_axis);

        Matrix4 translation;
        translation.identity();
        translation._14 = -pos.x;
        translation._24 = -pos.y;
        translation._34 = -pos.z;

        Matrix4 rotation;
        rotation.identity();
        rotation._11 = x_axis.x;
        rotation._12 = x_axis.y;
        rotation._13 = x_axis.z;
        rotation._21 = y_axis.x;
        rotation._22 = y_axis.y;
        rotation._23 = y_axis.z;
        rotation._31 = z_axis.x;
        rotation._32 = z_axis.y;
        rotation._33 = z_axis.z;

        *this = rotation * translation;
    }

    inline Matrix4 transpose() {
        Matrix4 result;
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                result.e[row][col] = e[col][row];
            }
        }
        return result;
    }
    
    inline Matrix4 operator*(Matrix4 b) {
        Matrix4 result;
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                float sum = 0.0f;
                for (int el = 0; el < 4; el++) {
                    sum += e[row][el] * b.e[el][col];
                }
                result.e[row][col] = sum;
            }
        }
        return result;        
    }
};

inline Matrix4 make_transformation_matrix(Vector3 position, Vector3 rotation, Vector3 scale) {
    Matrix4 m;
    m.identity();

    m._14 = position.x;
    m._24 = position.y;
    m._34 = position.z;

    m._11 = scale.x;
    m._22 = scale.y;
    m._33 = scale.z;

    Matrix4 rx;
    rx.x_rotation(rotation.x * (PI / 180.0f));

    Matrix4 ry;
    ry.y_rotation(rotation.y * (PI / 180.0f));

    Matrix4 rz;
    rz.z_rotation(rotation.z * (PI / 180.0f));

    return m * rx * ry * rz;
}

inline Matrix4 make_transformation_matrix(Vector3 position, Vector3 rotation, float scale) {
    return make_transformation_matrix(position, rotation, Vector3(scale, scale, scale));
}

inline Matrix4 make_orthographic(float l, float r, float b, float t) {
    Matrix4 result;
    result.identity();

    result._11 = 2.0f/(r-l);
    result._22 = 2.0f/(t-b);
    result._14 = -((r+l)/(r-l));
    result._24 = -((t+b)/(t-b));
    
    return result;
}

struct Quaternion {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float w = 1.0f;

    inline void set_from_axis_and_angle(Vector3 v, float angle) {
        float half_angle_radians = (angle * 0.5f) * (PI / 180.0f);

        float sine_half_angle = sinf(half_angle_radians);
        float cosine_half_angle = cosf(half_angle_radians);

        x = v.x * sine_half_angle;
        y = v.y * sine_half_angle;
        z = v.z * sine_half_angle;
        w = cosine_half_angle;
    }
    
    inline Quaternion conjugate() {
        Quaternion result;

        result.x = -x;
        result.y = -y;
        result.z = -z;
        result.w = w;
        
        return result;
    }
    
    inline void set_matrix(Matrix4 *m) {
        m->zero();

        float xy = x*y;
        float xz = x*z;
        float xw = x*w;
        float yz = y*z;
        float yw = y*w;
        float zw = z*w;
        float x_sq = x*x;
        float y_sq = y*y;
        float z_sq = z*z;
        
        m->_11 = 1 - 2 * (y_sq + z_sq);
        m->_21 = 2 * (xy - zw);
        m->_31 = 2 * (xz + yw);
        m->_41 = 0.0f;

        m->_12 = 2 * (xy + zw);
        m->_22 = 1.0f - 2.0f * (x_sq + z_sq);
        m->_32 = 2 * (yz - xw);
        m->_42 = 0.0f;

        m->_13 = 2 * (xz - yw);
        m->_23 = 2 * (yz + xw);
        m->_33 = 1.0f - 2.0f * (x_sq + y_sq);
        m->_43 = 0.0f;

        m->_14 = 0.0f;
        m->_24 = 0.0f;
        m->_34 = 0.0f;
        m->_44 = 1.0f;
    }

    inline Quaternion operator*(Quaternion b) {
        Quaternion result = {};
        
        result.w = (w * b.w) - (x * b.x) - (y * b.y) - (z * b.z);
        result.x = (x * b.w) + (w * b.x) + (y * b.z) - (z * b.y);
        result.y = (y * b.w) + (w * b.y) + (z * b.x) - (x * b.z);
        result.z = (z * b.w) + (w * b.z) + (x * b.y) - (y * b.x);

        return result;
    }

    inline Quaternion operator*(Vector3 b) {
        Quaternion result = {};
        
        result.w = - (x * b.x) - (y * b.y) - (z * b.z);
        result.x =   (w * b.x) + (y * b.z) - (z * b.y);
        result.y =   (w * b.y) + (z * b.x) - (x * b.z);
        result.z =   (w * b.z) + (x * b.y) - (y * b.x);
        
        return result;
    }
};

inline float length_squared(Quaternion q) {
    return q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
}

inline float length(Quaternion q) {
    return sqrtf(length_squared(q));
}

inline Quaternion normalize_or_zero(Quaternion q) {
    Quaternion result = {};
    
    float len_sq = length_squared(q);
    if (len_sq < 0.001f * 0.001f) result;
    float inv_len = 1.0f / sqrtf(len_sq);

    result.x = q.x * inv_len;
    result.y = q.y * inv_len;
    result.z = q.z * inv_len;
    result.w = q.w * inv_len;

    return result;
}

struct Rectangle2i {
    int x;
    int y;
    int width;
    int height;
};

inline Rectangle2i aspect_ratio_fit(int window_width, int window_height, int render_width, int render_height) {
    Rectangle2i result = {};
    if (!window_width || !window_height || !render_width || !render_height) return result;

    float optimal_window_width = (float)window_height * ((float)render_width / (float)render_height);
    float optimal_window_height = (float)window_width * ((float)render_height / (float)render_width);

    if ((float)window_width > optimal_window_width) {
        result.y = 0;
        result.height = (int)window_height;

        result.width = (int)optimal_window_width;
        result.x = (window_width - result.width) / 2;
    } else {
        result.x = 0;
        result.width = (int)window_width;

        result.height = (int)optimal_window_height;
        result.y = (window_height - result.height) / 2;
    }

    return result;
}

struct Rectangle2 {
    float x;
    float y;
    float width;
    float height;
};

inline bool is_touching_left(Rectangle2 a, Rectangle2 b, Vector2 vel) {
    float al = a.x;
    float ar = al + a.width;
    float ab = a.y;
    float at = a.y + a.height;
    
    float bl = b.x;
    float br = bl + b.width;
    float bb = b.y;
    float bt = b.y + b.height;

    return ar + vel.x > bl &&
        al < bl &&
        ab < bt &&
        at > bb;
}

inline bool is_touching_right(Rectangle2 a, Rectangle2 b, Vector2 vel) {
    float al = a.x;
    float ar = al + a.width;
    float ab = a.y;
    float at = a.y + a.height;

    float bl = b.x;
    float br = bl + b.width;
    float bb = b.y;
    float bt = b.y + b.height;

    return al + vel.x < br &&
        ar > br &&
        ab < bt &&
        at > bb;
}

inline bool is_touching_top(Rectangle2 a, Rectangle2 b, Vector2 vel) {
    float al = a.x;
    float ar = al + a.width;
    float ab = a.y;
    float at = a.y + a.height;

    float bl = b.x;
    float br = bl + b.width;
    float bb = b.y;
    float bt = b.y + b.height;

    return ab + vel.y < bt &&
        at > bt &&
        ar > bl &&
        al < br;
}

inline bool is_touching_bottom(Rectangle2 a, Rectangle2 b, Vector2 vel) {
    float al = a.x;
    float ar = al + a.width;
    float ab = a.y;
    float at = a.y + a.height;

    float bl = b.x;
    float br = bl + b.width;
    float bb = b.y;
    float bt = b.y + b.height;

    return at + vel.y > bb &&
        ab < bb &&
        ar > bl &&
        al < br;
}
