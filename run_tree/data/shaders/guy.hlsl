DepthTest = "Off"
DepthWrite = "Off"
AlphaBlend = "On"
CullFace = "Off"
FrontFaceIsCounterClockwise = "True"
VertexType = "XCUN"
RenderTopology = "TriangleList"
TextureFilter = "Point"
TextureWrap = "Repeat"

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
SamplerState tex_samp : register(s0);

float4 pixel_main(VSOutput input) : SV_TARGET {
#if 1
    float4 dif_col = dif_tex.Sample(tex_samp, input.uv);
    return input.color * dif_col;
#else
    const float offset = 1.0 / 128.0;
    
    float4 outcol;

    float4 col = dif_tex.Sample(tex_samp, input.uv);
	if (col.a > 0.5)
		outcol = col;
	else {
		float a = dif_tex.Sample(tex_samp, float2(input.uv.x + offset, input.uv.y)).a +
			dif_tex.Sample(tex_samp, float2(input.uv.x, input.uv.y - offset)).a +
			dif_tex.Sample(tex_samp, float2(input.uv.x - offset, input.uv.y)).a +
            dif_tex.Sample(tex_samp, float2(input.uv.x, input.uv.y + offset)).a;
		if (col.a < 1.0 && a > 0.0)
			outcol = float4(0.0, 0.0, 0.0, 0.8);
		else
			outcol = col;
	}

    outcol = outcol * input.color;
    return outcol;
#endif
}
