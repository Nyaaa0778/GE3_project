#include "FinalBlit.hlsli"

struct DissolveParams
{
    float32_t threshold;
    float32_t edgeWidth;
    float32_t4 edgeColor;
};
ConstantBuffer<DissolveParams> gDissolveParams : register(b0);

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t> gMaskTexture : register(t1);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float32_t mask = gMaskTexture.Sample(gSampler, input.texcoord).r;
    
    if (mask < gDissolveParams.threshold)
    {
        discard;
    }
    
    float4 screenColor = gTexture.Sample(gSampler, input.texcoord);
    
    // エッジ部分の発光処理 (しきい値からしきい値+エッジ幅の間)
    float32_t edgeThreshold = gDissolveParams.threshold + gDissolveParams.edgeWidth;
    if (mask < edgeThreshold)
    {
        float32_t edgeWeight = (edgeThreshold - mask) / gDissolveParams.edgeWidth;
        screenColor.rgb = lerp(screenColor.rgb, gDissolveParams.edgeColor.rgb, edgeWeight);
    }
    
    output.color = screenColor;
    return output;
}