#include "Skybox.hlsli"

// キューブマップとしてテクスチャを受け取る
TextureCube<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // 3次元のUV（texcoord）を使ってサンプリング
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    // ライティングなどはせず、画像の色をそのまま出力
    output.color = textureColor;
    
    return output;
}