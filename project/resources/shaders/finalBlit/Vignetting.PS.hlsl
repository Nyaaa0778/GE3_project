#include "FinalBlit.hlsli"

struct VignetteParams
{
    float32_t4 color;
};
ConstantBuffer<VignetteParams> gVignetteParams : register(b0);

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gTexture.Sample(gSampler, input.texcoord);
    
    // 周囲を 0 に、中心になるほど明るくなるように調整
    float32_t2 correct = input.texcoord * (1.0f - input.texcoord.yx);
    // 中心の最大値を調整
    float vignette = correct.x * correct.y * 16.0f;
    // 0.8乗でそれっぽく
    vignette = saturate(pow(vignette, 0.4f));
    // 係数として乗算
    output.color.rgb = lerp(gVignetteParams.color.rgb, output.color.rgb, vignette);
    
    return output;
}