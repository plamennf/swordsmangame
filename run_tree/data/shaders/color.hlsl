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
};

VSOutput vertex_main(float3 position : POSITION, float4 color : COLOR, float2 uv : TEXCOORD, float3 normal : NORMAL) {
    VSOutput output;

    output.position = mul(transform, float4(position, 1.0));
    output.color = color;

    return output;
}

float4 pixel_main(VSOutput input) : SV_TARGET {
    return input.color;
}
