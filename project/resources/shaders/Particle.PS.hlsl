#include "Particle.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // UV変換なし、そのままサンプリング
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    // テクスチャ色 × パーティクル色
    output.color = textureColor * input.color;

    return output;
}
