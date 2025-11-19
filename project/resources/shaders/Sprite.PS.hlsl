#include "Sprite.hlsli"

// スプライト用マテリアル
struct Material
{
    float32_t4 color; // 乗算カラー
    float32_t4x4 uvTransform; // UVアニメさせたいとき用
};
ConstantBuffer<Material> gMaterial : register(b0);

// SRV / サンプラ
Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

// 出力
struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput o;

    // UV変換
    float32_t4 uv = float32_t4(input.texcoord, 0.0f, 1.0f);
    uv = mul(uv, gMaterial.uvTransform);
    float32_t2 transformedUV = uv.xy;

    // テクスチャサンプル
    float32_t4 texColor = gTexture.Sample(gSampler, transformedUV);

    // 乗算カラー
    float32_t4 base = texColor * gMaterial.color;

    // 必要ならアルファテスト
    // if (base.a <= 0.0f) { discard };

    o.color = base;
    return o;
}