#pragma once

#include "render_vertex.h"

enum Depth_Test_Mode {
    DEPTH_TEST_OFF = 0,
    DEPTH_TEST_LESS = 1,
    DEPTH_TEST_GREATER = 2,
};

enum Cull_Mode {
    CULL_MODE_OFF = 0,
    CULL_MODE_BACK = 1,
    CULL_MODE_FRONT = 2,
};

enum Texture_Filter_Mode {
    TEXTURE_FILTER_LINEAR = 0,
    TEXTURE_FILTER_POINT = 1,
};

enum Texture_Wrap_Mode {
    TEXTURE_WRAP_REPEAT = 0,
    TEXTURE_WRAP_CLAMP = 1,
};

struct Texture_Sampler_Description {
    Texture_Filter_Mode texture_filter_mode;
    Texture_Wrap_Mode texture_wrap_mode;
};

// Copied from D3D11_BLEND
enum Blend_Mode {
    BLEND_ZERO = 1,
    BLEND_ONE = 2,
    BLEND_SRC_COLOR = 3,
    BLEND_INV_SRC_COLOR = 4,
    BLEND_SRC_ALPHA = 5,
    BLEND_INV_SRC_ALPHA = 6,
    BLEND_DEST_ALPHA = 7,
    BLEND_INV_DEST_ALPHA = 8,
    BLEND_DEST_COLOR = 9,
    BLEND_INV_DEST_COLOR = 10,
    BLEND_SRC_ALPHA_SAT = 11,
    BLEND_BLEND_FACTOR = 14,
    BLEND_INV_BLEND_FACTOR = 15,
    BLEND_SRC1_COLOR = 16,
    BLEND_INV_SRC1_COLOR = 17,
    BLEND_SRC1_ALPHA = 18,
    BLEND_INV_SRC1_ALPHA = 19
};

// Copied from D3D11_BLEND_OP
enum Blend_Op {
    BLEND_OP_ADD = 1,
    BLEND_OP_SUBTRACT = 2,
    BLEND_OP_REV_SUBTRACT = 3,
    BLEND_OP_MIN = 4,
    BLEND_OP_MAX = 5,
};

struct Blend_State_Description {
    bool blend_enabled = false;
    Blend_Mode src_blend = BLEND_SRC_ALPHA;
    Blend_Mode dest_blend = BLEND_INV_SRC_ALPHA;
    Blend_Op blend_op = BLEND_OP_ADD;
    Blend_Mode src_blend_alpha = BLEND_ZERO;
    Blend_Mode dest_blend_alpha = BLEND_ONE;
    Blend_Op blend_op_alpha = BLEND_OP_ADD;
    u8 rt_write_mask = 15; // 1 - Red | 2 - Green | 4 - Blue | 8 - Alpha | 15 - All
};

struct Shader_Options {
    Render_Vertex_Type vertex_type;
    Blend_State_Description blend;
    Cull_Mode cull_mode;
    bool front_face_is_counter_clockwise;
    Depth_Test_Mode depth_test_mode;
    bool depth_write_enabled;
    Array <Texture_Sampler_Description> samplers;
};

struct Shader {
    char *full_path;
    char *name;
    u64 modtime;

    Shader_Options options;
    
    ID3D11VertexShader *vs = 0;
    ID3D11PixelShader *ps = 0;
    ID3D11InputLayout *il = 0;
    ID3D11BlendState *blend_state = 0;
    ID3D11RasterizerState1 *rasterizer_state = 0;
    ID3D11DepthStencilState *depth_stencil_state = 0;
    D3D11_PRIMITIVE_TOPOLOGY topology;
    Array <ID3D11SamplerState *> sampler_states;
};

struct Global_Parameters {
    Matrix4 transform;
    Matrix4 proj_matrix;
    Matrix4 view_matrix;
    Vector2 light_position;
};

extern Global_Parameters global_parameters;

void refresh_global_parameters();
