#include "pch.h"
#include "render.h"
#include "array.h"
#include "game.h"

Color_Target *the_back_buffer = NULL;

Color_Target *the_offscreen_buffer = NULL;
Color_Target *the_lightmap_buffer = NULL;

static bool should_vsync;

static Shader *current_shader;

static ID3D11Device1 *device;
static ID3D11DeviceContext1 *device_context;
static IDXGISwapChain1 *swap_chain;

static const int MAX_IMMEDIATE_VERTICES = 2400;
static Vertex_XCUN immediate_vertices[MAX_IMMEDIATE_VERTICES];
static int num_immediate_vertices;

static ID3D11Buffer *immediate_vbo;

static ID3D11Buffer *global_parameters_cbo;

static void create_rtv() {
    ID3D11Texture2D *frame_buffer = NULL;
    defer { SafeRelease(frame_buffer); };
    swap_chain->GetBuffer(0, IID_PPV_ARGS(&frame_buffer));
    device->CreateRenderTargetView(frame_buffer, NULL, &the_back_buffer->rtv);
}

static void release_rtv() {
    SafeRelease(the_back_buffer->rtv);
}

void init_render(Window_Type window, int width, int height, bool vsync) {
    should_vsync = vsync;
    
    //
    // Create device and device context
    //
    {
        D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };

        ID3D11Device *base_device = NULL;
        defer { SafeRelease(base_device); };
        ID3D11DeviceContext *base_device_context = NULL;
        defer { SafeRelease(base_device_context); };

        UINT device_create_flags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
        device_create_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        
        D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, device_create_flags,
                          feature_levels, ArrayCount(feature_levels), D3D11_SDK_VERSION,
                          &base_device, NULL, &base_device_context);

        base_device->QueryInterface(IID_PPV_ARGS(&device));
        base_device_context->QueryInterface(IID_PPV_ARGS(&device_context));
    }

    //
    // Create swap chain
    //
    {
        IDXGIDevice1 *dxgi_device = NULL;
        defer { SafeRelease(dxgi_device); };
        device->QueryInterface(IID_PPV_ARGS(&dxgi_device));

        IDXGIAdapter *dxgi_adapter = NULL;
        defer { SafeRelease(dxgi_adapter); };
        dxgi_device->GetAdapter(&dxgi_adapter);

        IDXGIFactory2 *dxgi_factory = NULL;
        defer { SafeRelease(dxgi_factory); };
        dxgi_adapter->GetParent(IID_PPV_ARGS(&dxgi_factory));

        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
        swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        swap_chain_desc.SampleDesc.Count = 1;
        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.BufferCount = 2;
        swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swap_chain_desc.Flags = 0;

        dxgi_factory->CreateSwapChainForHwnd(device, window, &swap_chain_desc, NULL, NULL, &swap_chain);
        dxgi_factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
    }

    the_back_buffer = new Color_Target();
    create_rtv();

    D3D11_BUFFER_DESC immediate_vb_bd = {};
    immediate_vb_bd.ByteWidth = MAX_IMMEDIATE_VERTICES * sizeof(Vertex_XCUN);
    immediate_vb_bd.Usage = D3D11_USAGE_DYNAMIC;
    immediate_vb_bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    immediate_vb_bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&immediate_vb_bd, NULL, &immediate_vbo);

    num_immediate_vertices = 0;

    D3D11_BUFFER_DESC global_parameters_cb_bd = {};
    global_parameters_cb_bd.ByteWidth = sizeof(Global_Parameters) + 0xf & 0xfffffff0; // round constant buffer size up to 16 byte boundary
    global_parameters_cb_bd.Usage = D3D11_USAGE_DYNAMIC;
    global_parameters_cb_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    global_parameters_cb_bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&global_parameters_cb_bd, NULL, &global_parameters_cbo);

    /* @HACK
    D3D11_BLEND_DESC blend_desc = {};
    blend_desc.RenderTarget[0].BlendEnable = true;
    blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&blend_desc, &HACK_light_blend_state);
    */
}

void swap_buffers() {
    swap_chain->Present(should_vsync ? 1 : 0, 0);
}

void render_resize(int width, int height) {
    if (!swap_chain) return;

    if (the_lightmap_buffer) {
        release_color_target(the_lightmap_buffer);
        delete the_lightmap_buffer->texture;
        delete the_lightmap_buffer;
        the_lightmap_buffer = NULL; // This will get created again when computing render_area in do_one_frame
    }

    if (the_offscreen_buffer) {
        release_color_target(the_offscreen_buffer);
        delete the_offscreen_buffer->texture;
        delete the_offscreen_buffer;
        the_offscreen_buffer = NULL; // This will get created again when computing render_area in do_one_frame
    }
    
    release_rtv();
    swap_chain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    create_rtv();
}

Color_Target *create_color_target(int width, int height) {
    Color_Target *result = new Color_Target();
    result->texture = new Texture();
    
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = width;
    td.Height = height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    HRESULT hr = device->CreateTexture2D(&td, NULL, &result->texture->texture);
    AssertHR(hr);

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = td.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = 1;
    hr = device->CreateShaderResourceView(result->texture->texture, &srv_desc, &result->texture->srv);
    AssertHR(hr);
    
    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.Format = td.Format;
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtv_desc.Texture2D = D3D11_TEX2D_RTV{0};
    hr = device->CreateRenderTargetView(result->texture->texture, &rtv_desc, &result->rtv);
    AssertHR(hr);
    
    return result;
}

void release_color_target(Color_Target *ct) {
    SafeRelease(ct->texture->srv);
    SafeRelease(ct->texture->texture);
    SafeRelease(ct->rtv);
}

void set_render_targets(Color_Target *ct, Depth_Target *dt) {
    device_context->OMSetRenderTargets(ct ? 1 : 0, ct ? &ct->rtv : NULL, dt ? dt->dsv : NULL);
}

void clear_color_target(Color_Target *ct, float r, float g, float b, float a, Rectangle2i *rect) {
    if (ct) {
        float clear_color[4] = { r, g, b, a };

        D3D11_RECT drect;
        if (rect) {
            drect.left = rect->x;
            drect.right = rect->x + rect->width;
            drect.top = rect->y;
            drect.bottom = rect->y + rect->height;
        }
        
        device_context->ClearView(ct->rtv, clear_color, rect ? &drect : NULL, rect ? 1 : 0);
    }
}

void clear_depth_target(Depth_Target *dt, float z) {
    if (dt) {
        device_context->ClearDepthStencilView(dt->dsv, D3D11_CLEAR_DEPTH, z, 0);
    }
}

void immediate_begin() {
    immediate_flush();
}

void immediate_flush() {
    if (!num_immediate_vertices) return;

    D3D11_MAPPED_SUBRESOURCE msr;
    device_context->Map(immediate_vbo, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, immediate_vertices, num_immediate_vertices * sizeof(Vertex_XCUN));
    device_context->Unmap(immediate_vbo, 0);

    UINT offsets[1] = { 0 };
    UINT strides[1] = { sizeof(Vertex_XCUN) };
    device_context->IASetVertexBuffers(0, 1, &immediate_vbo, strides, offsets);

    device_context->Draw(num_immediate_vertices, 0);
    
    num_immediate_vertices = 0;
}

static void put_vertex(Vertex_XCUN *v, Vector3 position, Vector4 color) {
    v->position = position;
    v->color = color;
    v->uv = Vector2(0, 0);
    v->normal = Vector3(0, 0, 1);
}

void immediate_quad(Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3, Vector4 color) {
    if (num_immediate_vertices + 6 > MAX_IMMEDIATE_VERTICES) immediate_flush();

    Vertex_XCUN *v = immediate_vertices + num_immediate_vertices;
    
    put_vertex(&v[0], p0, color);
    put_vertex(&v[1], p1, color);
    put_vertex(&v[2], p2, color);
    
    put_vertex(&v[3], p0, color);
    put_vertex(&v[4], p2, color);
    put_vertex(&v[5], p3, color);
    
    num_immediate_vertices += 6;
}

static void put_vertex(Vertex_XCUN *v, Vector2 position, Vector4 color) {
    v->position.x = position.x;
    v->position.y = position.y;
    v->position.z = 0.0f;
    v->color = color;
    v->uv = Vector2(0, 0);
    v->normal = Vector3(0, 0, 1);
}

static void put_vertex(Vertex_XCUN *v, Vector2 position, Vector2 uv, Vector4 color) {
    v->position.x = position.x;
    v->position.y = position.y;
    v->position.z = 0.0f;
    v->color = color;
    v->uv = uv;
    v->normal = Vector3(0, 0, 1);
}

void immediate_quad(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, Vector4 color) {
    if (num_immediate_vertices + 6 > MAX_IMMEDIATE_VERTICES) immediate_flush();

    Vertex_XCUN *v = immediate_vertices + num_immediate_vertices;
    
    put_vertex(&v[0], p0, color);
    put_vertex(&v[1], p1, color);
    put_vertex(&v[2], p2, color);
    
    put_vertex(&v[3], p0, color);
    put_vertex(&v[4], p2, color);
    put_vertex(&v[5], p3, color);
    
    num_immediate_vertices += 6;
}

void immediate_quad(float x0, float y0, float x1, float y1, Vector4 color) {
    Vector2 p0(x0, y0);
    Vector2 p1(x1, y0);
    Vector2 p2(x1, y1);
    Vector2 p3(x0, y1);
    
    immediate_quad(p0, p1, p2, p3, color);
}

void immediate_quad(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, Vector2 uv0, Vector2 uv1, Vector2 uv2, Vector2 uv3, Vector4 color) {
    if (num_immediate_vertices + 6 > MAX_IMMEDIATE_VERTICES) immediate_flush();

    Vertex_XCUN *v = immediate_vertices + num_immediate_vertices;

    put_vertex(&v[0], p0, uv0, color);
    put_vertex(&v[1], p1, uv1, color);
    put_vertex(&v[2], p2, uv2, color);
    
    put_vertex(&v[3], p0, uv0, color);
    put_vertex(&v[4], p2, uv2, color);
    put_vertex(&v[5], p3, uv3, color);
    
    num_immediate_vertices += 6;
}

void immediate_quad(float x0, float y0, float x1, float y1, Vector2 uv0, Vector2 uv1, Vector2 uv2, Vector2 uv3, Vector4 color) {
    Vector2 p0(x0, y0);
    Vector2 p1(x1, y0);
    Vector2 p2(x1, y1);
    Vector2 p3(x0, y1);

    immediate_quad(p0, p1, p2, p3, uv0, uv1, uv2, uv3, color);
}

void immediate_triangle(Vector2 p0, Vector2 p1, Vector2 p2, Vector4 color) {
    if (num_immediate_vertices + 3 > MAX_IMMEDIATE_VERTICES) immediate_flush();

    Vertex_XCUN *v = immediate_vertices + num_immediate_vertices;

    put_vertex(&v[0], p0, color);
    put_vertex(&v[1], p1, color);
    put_vertex(&v[2], p2, color);
    
    num_immediate_vertices += 3;
}

Shader *immediate_set_shader(Shader *shader) {
    assert(shader);
    
    if (current_shader) immediate_flush();
    
    current_shader = shader;

    device_context->VSSetShader(shader->vs, NULL, 0);
    device_context->PSSetShader(shader->ps, NULL, 0);
    device_context->IASetInputLayout(shader->il);
    if (globals.render_type == RENDER_TYPE_LIGHTS) {
        device_context->OMSetBlendState(NULL, NULL, 0xFFFFFFFF);
    } else {
        device_context->OMSetBlendState(shader->blend_state, NULL, 0xFFFFFFFF);
    }
    device_context->RSSetState(shader->rasterizer_state);
    device_context->OMSetDepthStencilState(shader->depth_stencil_state, 0);
    device_context->IASetPrimitiveTopology(shader->topology);
    device_context->PSSetSamplers(0, shader->sampler_states.count, shader->sampler_states.data);

    ID3D11Buffer *cbs[] = { global_parameters_cbo };
    
    device_context->VSSetConstantBuffers(0, ArrayCount(cbs), cbs);
    
    return current_shader;
}

static D3D11_CULL_MODE d3d11_cull_mode(Cull_Mode cull_mode) {
    switch (cull_mode) {
    case CULL_MODE_OFF: return D3D11_CULL_NONE;
    case CULL_MODE_FRONT: return D3D11_CULL_FRONT;
    case CULL_MODE_BACK: return D3D11_CULL_BACK;
    }
    return D3D11_CULL_NONE;
}

static D3D11_COMPARISON_FUNC d3d11_depth_func(Depth_Test_Mode mode) {
    switch (mode) {
    case DEPTH_TEST_LESS: return D3D11_COMPARISON_LESS;
    case DEPTH_TEST_GREATER: return D3D11_COMPARISON_GREATER;
    }
    return D3D11_COMPARISON_LESS;
}

bool load_shader_from_memory(Shader *shader, char *preprocessed_text, char *filepath) {
    ID3DBlob *code_blob, *error_blob;
    defer { SafeRelease(code_blob); SafeRelease(error_blob); };
    D3DCompile(preprocessed_text, string_length(preprocessed_text), NULL, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "vertex_main", "vs_5_0", 0, 0, &code_blob, &error_blob);
    if (error_blob) {
        log_error("Failed to compile '%s' vertex shader:\n%s\n", filepath, (char *)error_blob->GetBufferPointer());
        return false;
    }
    device->CreateVertexShader(code_blob->GetBufferPointer(), code_blob->GetBufferSize(), NULL, &shader->vs);

    Shader_Options options = shader->options;
    
    D3D11_INPUT_ELEMENT_DESC ieds[16] = {};
    int num_ieds = 0;
    
    switch (options.vertex_type) {
    case RENDER_VERTEX_XCUN:
        num_ieds = 4;

        ieds[0].SemanticName = "POSITION";
        ieds[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        ieds[0].AlignedByteOffset = offsetof(Vertex_XCUN, position);
        ieds[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

        ieds[1].SemanticName = "COLOR";
        ieds[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        ieds[1].AlignedByteOffset = offsetof(Vertex_XCUN, color);
        ieds[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

        ieds[2].SemanticName = "TEXCOORD";
        ieds[2].Format = DXGI_FORMAT_R32G32_FLOAT;
        ieds[2].AlignedByteOffset = offsetof(Vertex_XCUN, uv);
        ieds[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

        ieds[3].SemanticName = "NORMAL";
        ieds[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        ieds[3].AlignedByteOffset = offsetof(Vertex_XCUN, normal);
        ieds[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        
        break;
    }

    device->CreateInputLayout(ieds, num_ieds, code_blob->GetBufferPointer(), code_blob->GetBufferSize(), &shader->il);

    SafeRelease(code_blob);
    SafeRelease(error_blob);

    D3DCompile(preprocessed_text, string_length(preprocessed_text), NULL, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "pixel_main", "ps_5_0", 0, 0, &code_blob, &error_blob);
    if (error_blob) {
        log_error("Failed to compile '%s' pixel shader:\n%s\n", filepath, (char *)error_blob->GetBufferPointer());
        return false;
    }
    device->CreatePixelShader(code_blob->GetBufferPointer(), code_blob->GetBufferSize(), NULL, &shader->ps);
    
    D3D11_RASTERIZER_DESC1 rasterizer_desc = {};
    rasterizer_desc.FillMode = D3D11_FILL_SOLID;
    rasterizer_desc.CullMode = d3d11_cull_mode(options.cull_mode);
    rasterizer_desc.FrontCounterClockwise = options.front_face_is_counter_clockwise;
    rasterizer_desc.ScissorEnable = true;
    device->CreateRasterizerState1(&rasterizer_desc, &shader->rasterizer_state);

    D3D11_DEPTH_STENCIL_DESC ds_desc = {};
    ds_desc.DepthEnable = options.depth_test_mode != DEPTH_TEST_OFF;
    ds_desc.DepthWriteMask = options.depth_write_enabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    ds_desc.DepthFunc = d3d11_depth_func(options.depth_test_mode);
    device->CreateDepthStencilState(&ds_desc, &shader->depth_stencil_state);

    D3D11_BLEND_DESC blend_desc = {};
    blend_desc.RenderTarget[0].BlendEnable = (D3D11_BLEND)options.blend.blend_enabled;
    blend_desc.RenderTarget[0].SrcBlend = (D3D11_BLEND)options.blend.src_blend;
    blend_desc.RenderTarget[0].DestBlend = (D3D11_BLEND)options.blend.dest_blend;
    blend_desc.RenderTarget[0].BlendOp = (D3D11_BLEND_OP)options.blend.blend_op;
    blend_desc.RenderTarget[0].SrcBlendAlpha = (D3D11_BLEND)options.blend.src_blend_alpha;
    blend_desc.RenderTarget[0].DestBlendAlpha = (D3D11_BLEND)options.blend.dest_blend_alpha;
    blend_desc.RenderTarget[0].BlendOpAlpha = (D3D11_BLEND_OP)options.blend.blend_op_alpha;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = options.blend.rt_write_mask;
    device->CreateBlendState(&blend_desc, &shader->blend_state);

    for (auto const &desc : options.samplers) {
        D3D11_SAMPLER_DESC sampler_desc = {};
        sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
        sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

        switch (desc.texture_filter_mode) {
            case TEXTURE_FILTER_LINEAR:
                sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                break;

            case TEXTURE_FILTER_POINT:
                sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
                break;
        }

        switch (desc.texture_wrap_mode) {
            case TEXTURE_WRAP_REPEAT:
                sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
                sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
                sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
                break;

            case TEXTURE_WRAP_CLAMP:
                sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
                sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
                sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
                break;
        }

        device->CreateSamplerState(&sampler_desc, shader->sampler_states.add());
    }
        
    return true;
}

static bool parse_blend_mode(char *line, Blend_Mode *mode) {
    if (strings_match(line, "Zero") || strings_match(line, "zero")) {
        *mode = BLEND_ZERO;
    } else if (strings_match(line, "One") || strings_match(line, "one")) {
        *mode = BLEND_ONE;
    } else if (strings_match(line, "SrcColor") || strings_match(line, "srccolor")) {
        *mode = BLEND_SRC_COLOR;
    } else if (strings_match(line, "InvSrcColor") || strings_match(line, "invsrccolor")) {
        *mode = BLEND_INV_SRC_COLOR;
    } else if (strings_match(line, "SrcAlpha") || strings_match(line, "srcalpha")) {
        *mode = BLEND_SRC_ALPHA;
    } else if (strings_match(line, "InvSrcAlpha") || strings_match(line, "invsrcalpha")) {
        *mode = BLEND_INV_SRC_ALPHA;
    } else if (strings_match(line, "DestAlpha") || strings_match(line, "destalpha")) {
        *mode = BLEND_DEST_ALPHA;
    } else if (strings_match(line, "InvDestAlpha") || strings_match(line, "invdestalpha")) {
        *mode = BLEND_INV_DEST_ALPHA;
    } else if (strings_match(line, "DestColor") || strings_match(line, "destcolor")) {
        *mode = BLEND_DEST_COLOR;
    } else if (strings_match(line, "InvDestColor") || strings_match(line, "invdestcolor")) {
        *mode = BLEND_INV_DEST_COLOR;
    } else if (strings_match(line, "SrcAlphaSat") || strings_match(line, "srcalphasat")) {
        *mode = BLEND_SRC_ALPHA_SAT;
    } else if (strings_match(line, "BlendFactor") || strings_match(line, "blendfactor")) {
        *mode = BLEND_BLEND_FACTOR;
    } else if (strings_match(line, "InvBlendFactor") || strings_match(line, "invblendfactor")) {
        *mode = BLEND_INV_BLEND_FACTOR;
    } else if (strings_match(line, "Src1Color") || strings_match(line, "src1color")) {
        *mode = BLEND_SRC1_COLOR;
    } else if (strings_match(line, "InvSrc1Color") || strings_match(line, "invsrc1color")) {
        *mode = BLEND_INV_SRC1_COLOR;
    } else if (strings_match(line, "Src1Alpha") || strings_match(line, "src1alpha")) {
        *mode = BLEND_SRC1_ALPHA;
    } else if (strings_match(line, "InvSrc1Alpha") || strings_match(line, "invsrc1alpha")) {
        *mode = BLEND_INV_SRC1_ALPHA;
    } else {
        log_error("Blend mode '%s' not supported\n", line);
        log_error("Valid values are:\n");
        log_error("    Zero\n");
        log_error("    One\n");
        log_error("    SrcColor\n");
        log_error("    InvSrcColor\n");
        log_error("    SrcAlpha\n");
        log_error("    InvSrcAlpha\n");
        log_error("    DestAlpha\n");
        log_error("    InvDestAlpha\n");
        log_error("    DestColor\n");
        log_error("    InvDestColor\n");
        log_error("    SrcAlphaSat\n");
        log_error("    BlendFactor\n");
        log_error("    InvBlendFactor\n");
        log_error("    Src1Color\n");
        log_error("    InvSrc1Color\n");
        log_error("    Src1Alpha\n");
        log_error("    InvSrc1Alpha\n");
        return false;
    }
    return true;
}

static bool parse_blend_op(char *line, Blend_Op *mode) {
    if (strings_match(line, "Add") || strings_match(line, "add")) {
        *mode = BLEND_OP_ADD;
    } else if (strings_match(line, "Subtract") || strings_match(line, "subtract")) {
        *mode = BLEND_OP_SUBTRACT;
    } else if (strings_match(line, "RevSubtract") || strings_match(line, "revsubtract")) {
        *mode = BLEND_OP_REV_SUBTRACT;
    } else if (strings_match(line, "Min") || strings_match(line, "min")) {
        *mode = BLEND_OP_MIN;
    } else if (strings_match(line, "Max") || strings_match(line, "max")) {
        *mode = BLEND_OP_MAX;
    } else {
        log_error("Blend op '%s' not supported\n", line);
        log_error("Valid values are:\n");
        log_error("    Add\n");
        log_error("    Subtract\n");
        log_error("    RevSubtract\n");
        log_error("    Min\n");
        log_error("    Max\n");
        return false;
    }
    return true;
}

static char *preprocess_shader(Shader *shader, char *data) {
    Array <char *> lines;
    defer {
        for (int i = 0; i < lines.count; i++) {
            delete [] lines[i];
        }
    };

    Shader_Options options = {};
    
    char *at = data;
    while (true) {
        char *line = consume_next_line(&at);
        if (!line) break;

        line = eat_spaces(line);
        line = eat_trailing_spaces(line);

        if (starts_with(line, "DepthTest")) {
            line += 9;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after DepthTest\n");
                return NULL;
            }
            line++;

            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            assert(line[0] == '"');
            line++;
            
            char *quote = strrchr(line, '"');
            if (quote) {
                line[quote - line] = 0;
            }
            
            if (strings_match(line, "Off") || strings_match(line, "off")) {
                options.depth_test_mode = DEPTH_TEST_OFF;
            } else if (strings_match(line, "Less") || strings_match(line, "less")) {
                options.depth_test_mode = DEPTH_TEST_LESS;
            } else if (strings_match(line, "Greater") || strings_match(line, "greater")) {
                options.depth_test_mode = DEPTH_TEST_GREATER;
            } else {
                log_error("DepthTest mode '%s' not supported\n", line);
                log_error("Valid values are:\n");
                log_error("    Off\n");
                log_error("    Less\n");
                log_error("    Greater\n");
                return NULL;
            }
        } else if (starts_with(line, "DepthWrite")) {
            line += 10;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after DepthWrite\n");
                return NULL;
            }
            line++;

            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            assert(line[0] == '"');
            line++;
            
            char *quote = strrchr(line, '"');
            if (quote) {
                line[quote - line] = 0;
            }
            
            if (strings_match(line, "Off") || strings_match(line, "off")) {
                options.depth_write_enabled = false;
            } else if (strings_match(line, "On") || strings_match(line, "on")) {
                options.depth_write_enabled = true;
            } else {
                log_error("DepthWrite mode '%s' not supported\n", line);
                log_error("Valid values are:\n");
                log_error("    On\n");
                log_error("    Off\n");
                return NULL;
            }
        } else if (starts_with(line, "AlphaBlend")) {
            line += 10;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after AlphaBlend\n");                
                return NULL;
            }
            line++;
            
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            assert(line[0] == '"');
            line++;
            
            char *quote = strrchr(line, '"');
            if (quote) {
                line[quote - line] = 0;
            }
            
            if (strings_match(line, "Off") || strings_match(line, "off")) {
                options.blend.blend_enabled = false;
            } else if (strings_match(line, "On") || strings_match(line, "on")) {
                options.blend.blend_enabled = true;
            } else {
                log_error("AlphaBlend mode '%s' not supported\n", line);
                log_error("Valid values are:\n");
                log_error("    On\n");
                log_error("    Off\n");
                return NULL;
            }
        } else if (starts_with(line, "SrcBlendAlpha")) {
            line += 13;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after SrcBlendAlpha\n");                
                return NULL;
            }
            line++;
            
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            assert(line[0] == '"');
            line++;
            
            char *quote = strrchr(line, '"');
            if (quote) {
                line[quote - line] = 0;
            }

            if (!parse_blend_mode(line, &options.blend.src_blend_alpha)) {
                return NULL;
            }
        } else if (starts_with(line, "DestBlendAlpha")) {
            line += 14;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after DestBlendAlpha\n");                
                return NULL;
            }
            line++;
            
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            assert(line[0] == '"');
            line++;
            
            char *quote = strrchr(line, '"');
            if (quote) {
                line[quote - line] = 0;
            }

            if (!parse_blend_mode(line, &options.blend.dest_blend_alpha)) {
                return NULL;
            }
        } else if (starts_with(line, "SrcBlend")) {
            line += 8;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after SrcBlend\n");                
                return NULL;
            }
            line++;
            
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            assert(line[0] == '"');
            line++;
            
            char *quote = strrchr(line, '"');
            if (quote) {
                line[quote - line] = 0;
            }

            if (!parse_blend_mode(line, &options.blend.src_blend)) {
                return NULL;
            }
        } else if (starts_with(line, "DestBlend")) {
            line += 9;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after DestBlend\n");                
                return NULL;
            }
            line++;
            
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            assert(line[0] == '"');
            line++;
            
            char *quote = strrchr(line, '"');
            if (quote) {
                line[quote - line] = 0;
            }

            if (!parse_blend_mode(line, &options.blend.dest_blend)) {
                return NULL;
            }
        } else if (starts_with(line, "BlendOpAlpha")) {
            line += 12;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after BlendOpAlpha\n");                
                return NULL;
            }
            line++;
            
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            assert(line[0] == '"');
            line++;
            
            char *quote = strrchr(line, '"');
            if (quote) {
                line[quote - line] = 0;
            }

            if (!parse_blend_op(line, &options.blend.blend_op_alpha)) {
                return NULL;
            }
        } else if (starts_with(line, "BlendOp")) {
            line += 7;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after BlendOp\n");                
                return NULL;
            }
            line++;
            
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            assert(line[0] == '"');
            line++;
            
            char *quote = strrchr(line, '"');
            if (quote) {
                line[quote - line] = 0;
            }

            if (!parse_blend_op(line, &options.blend.blend_op)) {
                return NULL;
            }
        } else if (starts_with(line, "RTWriteMask")) {
            line += 11;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after SrcBlendAlphaRTWriteMask\n");                
                return NULL;
            }
            line++;
            
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            options.blend.rt_write_mask = atoi(line);
        } else if (starts_with(line, "CullFace")) {
            line += 8;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after CullFace\n");
                return NULL;
            }
            line++;

            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            assert(line[0] == '"');
            line++;
            
            char *quote = strrchr(line, '"');
            if (quote) {
                line[quote - line] = 0;
            }
            
            if (strings_match(line, "Off") || strings_match(line, "off")) {
                options.cull_mode = CULL_MODE_OFF;
            } else if (strings_match(line, "Back") || strings_match(line, "back")) {
                options.cull_mode = CULL_MODE_BACK;
            } else if (strings_match(line, "Front") || strings_match(line, "front")) {
                options.cull_mode = CULL_MODE_FRONT;
            } else {
                log_error("CullFace mode '%s' not supported\n", line);
                log_error("Valid values are:\n");
                log_error("    Off\n");
                log_error("    Back\n");
                log_error("    Front\n");
                return NULL;
            }
        } else if (starts_with(line, "FrontFaceIsCounterClockwise")) {
            line += 27;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after FrontFaceIsCounterClockwise\n");
                return NULL;
            }
            line++;

            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            assert(line[0] == '"');
            line++;
            
            char *quote = strrchr(line, '"');
            if (quote) {
                line[quote - line] = 0;
            }
            
            if (strings_match(line, "true") || strings_match(line, "True")) {
                options.front_face_is_counter_clockwise = true;
            } else if (strings_match(line, "false") || strings_match(line, "False")) {
                options.front_face_is_counter_clockwise = false;
            } else {
                log_error("FrontFaceIsCounterClockwise mode '%s' not supported\n", line);
                log_error("Valid values are:\n");
                log_error("    True\n");
                log_error("    False\n");
                return NULL;
            }
        } else if (starts_with(line, "VertexType")) {
            line += 10;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after VertexType");
                return NULL;
            }
            line++;

            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            assert(line[0] == '"');
            line++;
            
            char *quote = strrchr(line, '"');
            if (quote) {
                line[quote - line] = 0;
            }
            
            if (strings_match(line, "XCUN") || strings_match(line, "xcun")) {
                options.vertex_type = RENDER_VERTEX_XCUN;
            } else {
                log_error("VertexType mode '%s' not supported\n", line);
                log_error("Valid values are:\n");
                log_error("    XCUN\n");
                return NULL;
            }
        } else if (starts_with(line, "RenderTopology")) {
            line += 14;
            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after VertexType");
                return NULL;
            }
            line++;

            line = eat_spaces(line);
            line = eat_trailing_spaces(line);

            assert(line[0] == '"');
            line++;
            
            char *quote = strrchr(line, '"');
            if (quote) {
                line[quote - line] = 0;
            }
            
            if (strings_match(line, "TriangleList")) {
                shader->topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            } else {
                log_error("RenderTopology mode '%s' not supported\n", line);
                log_error("Valid values are:\n");
                log_error("    TriangleList\n");
                return NULL;
            }
        } else if (starts_with(line, "TextureFilter")) {
            log_error("!!! Deprecated use sampler attributes !!!\n");
        } else if (starts_with(line, "TextureWrap")) {
            log_error("!!! Deprecated use sampler attributes !!!\n");
        } else if (starts_with(line, "SamplerState")) {
            char *at = strrchr(line, ';');
            if (!at) {
                log_error("Expected ; after SamplerState but found: %s\n", line);
                return NULL;
            }
            
            at++;
            Texture_Sampler_Description *desc = options.samplers.add();
            if (!*at) {
                desc->texture_filter_mode = TEXTURE_FILTER_LINEAR;
                desc->texture_wrap_mode = TEXTURE_WRAP_REPEAT;
                line[at - line] = 0;
                lines.add(copy_string(line));
                continue;
            }
            
            line[at - line] = 0;
            lines.add(copy_string(line));
            at++;
            
            while (*at) {
                at = eat_spaces(at);
                at = eat_trailing_spaces(at);
                
                if (*at == '@') {
                    char attrib[4096] = {};
                    sscanf(at, "@%s ", attrib);
                    at++;

                    if (strings_match(attrib, "Point") || strings_match(attrib, "point")) {
                        desc->texture_filter_mode = TEXTURE_FILTER_POINT;
                        at += 5;
                    } else if (strings_match(attrib, "Linear") || strings_match(attrib, "linear")) {
                        desc->texture_filter_mode = TEXTURE_FILTER_LINEAR;
                        at += 6;
                    } else if (strings_match(attrib, "Repeat") || strings_match(attrib, "repeat")) {
                        desc->texture_wrap_mode = TEXTURE_WRAP_REPEAT;
                        at += 6;
                    } else if (strings_match(attrib, "Clamp") || strings_match(attrib, "clamp")) {
                        desc->texture_wrap_mode = TEXTURE_WRAP_CLAMP;
                        at += 5;
                    } else {
                        log_error("Unknown sampler attribute '%s'\n", attrib);
                        log_error("Valid values are:\n");
                        log_error("    Point\n");
                        log_error("    Linear\n");
                        log_error("    Clamp\n");
                        return NULL;
                    }
                } else {
                    at++;
                }
            }
        } else {
            lines.add(copy_string(line));
        }
    }

    shader->options = options;
    options.samplers.data = NULL;
    
    return concatenate_with_newlines(lines.data, lines.count);
}

char *read_entire_text_file(char *filepath) {
    char *result = NULL;

    FILE *file = fopen(filepath, "rt");
    if (file) {
        fseek(file, 0, SEEK_END);
        size_t length = ftell(file);
        fseek(file, 0, SEEK_SET);

        result = new char[length + 1];
        size_t num_read = fread(result, 1, length, file);
        result[num_read] = 0;
        fclose(file);
    }
    return result;
}

bool load_shader(Shader *shader, char *filepath) {
    char *data = read_entire_text_file(filepath);
    if (!data) return false;
    defer { delete [] data; };

    char *preprocessed_text = preprocess_shader(shader, data);
    if (!preprocessed_text) return false;
    defer { delete [] preprocessed_text; };

    bool success = load_shader_from_memory(shader, preprocessed_text, filepath);
    return success;
}

void refresh_global_parameters() {
    D3D11_MAPPED_SUBRESOURCE msr;
    device_context->Map(global_parameters_cbo, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &global_parameters, sizeof(global_parameters));
    device_context->Unmap(global_parameters_cbo, 0);
}

void set_viewport(int x, int y, int width, int height) {
    D3D11_VIEWPORT vp;
    vp.TopLeftX = (float)x;
    vp.TopLeftY = (float)y;
    vp.Width = (float)width;
    vp.Height = (float)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    device_context->RSSetViewports(1, &vp);
}

void set_scissor(int x, int y, int width, int height) {
    D3D11_RECT rect;
    rect.left = x;
    rect.right = x + width;
    rect.top = y;
    rect.bottom = y + height;
    device_context->RSSetScissorRects(1, &rect);
}

bool load_texture_from_bitmap(Texture *texture, Bitmap *bitmap) {
    assert(bitmap->format != TEXTURE_FORMAT_UNKNOWN);

    texture->width = bitmap->width;
    texture->height = bitmap->height;
    texture->format = bitmap->format;
    texture->bytes_per_pixel = bitmap->bytes_per_pixel;
    
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    switch (bitmap->format) {
    case TEXTURE_FORMAT_RGBA8:
        format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        break;

    case TEXTURE_FORMAT_R8:
        format = DXGI_FORMAT_R8_UNORM;
        break;
    }

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = bitmap->width;
    td.Height = bitmap->height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = format;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA srd = {};
    srd.pSysMem = bitmap->data;
    srd.SysMemPitch = bitmap->width * bitmap->bytes_per_pixel;

    device->CreateTexture2D(&td, bitmap->data ? &srd : NULL, &texture->texture);

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = td.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(texture->texture, &srv_desc, &texture->srv);

    return true;
}

bool load_texture_from_file(Texture *texture, char *filepath) {
    Bitmap bitmap;
    if (!load_bitmap(&bitmap, filepath)) {
        log_error("Failed to load bitmap '%s'.\n", filepath);
        return false;
    }
    defer { deinit(&bitmap); };
    bool loaded_a_texture = load_texture_from_bitmap(texture, &bitmap);
    return loaded_a_texture;
}

void set_texture(int slot, Texture *texture) {
    device_context->PSSetShaderResources(slot, 1, &texture->srv);
}

void update_texture(Texture *texture, int x, int y, int width, int height, u8 *data) {
    D3D11_BOX box;
    box.left = x;
    box.right = box.left + width;
    box.top = y;
    box.bottom = box.top + height;
    box.front = 0;
    box.back = 1;
    
    device_context->UpdateSubresource(texture->texture, 0, &box, data, width * texture->bytes_per_pixel, 0);
}

