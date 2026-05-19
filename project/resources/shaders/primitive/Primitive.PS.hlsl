#include "Primitive.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float32_t4 color;
    float32_t4x4 uvTransform;
    float32_t alphaReference; 
};

ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float32_t2 texcoord = input.texcoord;
    texcoord.y = 1.0f - texcoord.y; // flip v
    
    float32_t4 transformedUV = mul(float32_t4(texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    // 1. テクスチャから色をサンプリング
    //float32_t4 textureColor = gTexture.Sample(gSampler, texcoord);
    
    // 2. 最終的な色を計算
    output.color = textureColor * gMaterial.color;
    
    // 3. discardの判定
    if (output.color.a <= gMaterial.alphaReference)
    {
        discard;
    }
    
    return output;
}