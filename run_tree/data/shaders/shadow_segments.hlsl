DepthTest = "Off"
DepthWrite = "Off"
AlphaBlend = "On"
CullFace = "Front"
FrontFaceIsCounterClockwise = "True"
VertexType = "XCUN"
RenderTopology = "TriangleList"
SrcBlend = "Zero"
DestBlend = "One"
SrcBlendAlpha = "Zero"
DestBlendAlpha = "Zero"
    
#include "data/shaders/shader_globals.hlsli"

struct VSOutput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VSOutput vertex_main(float3 position : POSITION, float4 color : COLOR, float2 uv : TEXCOORD, float3 normal : NORMAL) {
    VSOutput output;

    output.position = mul(transform, float4(position.xy - position.z*light_position, 0, 1 - position.z));
    output.color = color;

    return output;
}

float4 pixel_main(VSOutput input) : SV_TARGET {
    return input.color;
}
