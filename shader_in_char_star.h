#pragma once

const char VertexShader[] = R"(
struct VertexOutput
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};
VertexOutput Vertex_Shader(float2 pos : POSITION, float2 tex : TEXCOORD0)
{
    VertexOutput output;
    output.pos = float4(pos.x, pos.y, 0.f, 1.f);
    output.tex = tex;
    return output;
}
)";

const char PixelShader[] = R"(
struct PixelInput
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};
Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);
float4 Pixel_Shader(PixelInput input) : SV_TARGET
{
    return shaderTexture.Sample(SampleType, input.tex);
}
)";
