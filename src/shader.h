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

struct Shader_Options {
    Render_Vertex_Type vertex_type;
    bool alpha_blend_enabled;
    Cull_Mode cull_mode;
    bool front_face_is_counter_clockwise;
    Depth_Test_Mode depth_test_mode;
    bool depth_write_enabled;
    Texture_Filter_Mode texture_filter_mode;
    Texture_Wrap_Mode texture_wrap_mode;
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
    ID3D11SamplerState *sampler_state = 0;
};

struct Global_Parameters {
    Matrix4 transform;
    Matrix4 proj_matrix;
    Matrix4 view_matrix;
};

extern Global_Parameters global_parameters;

void refresh_global_parameters();
