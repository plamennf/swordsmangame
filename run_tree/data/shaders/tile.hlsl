/* Pipeline
DepthTest = "Off"
DepthWrite = "Off"
AlphaBlend = "On"
CullFace = "Off"
FrontFaceIsCounterClockwise = "True"
VertexType = "XCUN"
RenderTopology = "TriangleList"
*/

#include "data/shaders/shader_globals.hlsli"

struct VSOutput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV;

    float3 world_position : POSITION;
};

VSOutput vertex_main(float3 position : POSITION, float4 color : COLOR, float2 uv : TEXCOORD, float3 normal : NORMAL) {
    VSOutput output;

    output.position = mul(transform, float4(position, 1.0));
    output.color = color;
    output.uv = uv;

    output.world_position = position;

    return output;
}

Texture2D tex : register(t0);
SamplerState tex_samp : register(s0); @Point @Repeat

float4 pixel_main(VSOutput input) : SV_TARGET {
    float4 tex_col = tex.Sample(tex_samp, input.uv);
    return input.color * tex_col;
}
