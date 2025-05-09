DepthTest = "Off"
DepthWrite = "Off"
AlphaBlend = "On"
CullFace = "Off"
FrontFaceIsCounterClockwise = "True"
VertexType = "XCUN"
RenderTopology = "TriangleList"

#include "data/shaders/shader_globals.hlsli"

struct VSOutput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV;
};

VSOutput vertex_main(float3 position : POSITION, float4 color : COLOR, float2 uv : TEXCOORD, float3 normal : NORMAL) {
    VSOutput output;

    output.position = mul(transform, float4(position, 1.0));
    output.color = color;
    output.uv = uv;

    return output;
}

Texture2D dif_tex : register(t0);
Texture2D lightmap : register(t1);
SamplerState tex_samp : register(s0); @Linear @Repeat

float4 pixel_main(VSOutput input) : SV_TARGET {
    float4 dif_col = dif_tex.Sample(tex_samp, input.uv);
    float4 lightmap_col = lightmap.Sample(tex_samp, input.uv);
    return input.color * float4(dif_col.xyz * lightmap_col.xyz, 1);
}
