#include "FinalBlit.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct GrayscaleParams
{
    float32_t factor;
};
ConstantBuffer<GrayscaleParams> gGrayscaleParams : register(b0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t4 originalColor = gTexture.Sample(gSampler, input.texcoord);
    float32_t value = dot(originalColor.rgb, float32_t3(0.2125f, 0.7154f, 0.0721f));
    float32_t3 grayscaleColor = float32_t3(value, value, value);
    
    output.color.rgb = lerp(originalColor.rgb, grayscaleColor, gGrayscaleParams.factor);
    output.color.a = originalColor.a;
    return output;
}