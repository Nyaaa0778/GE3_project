#include"Object3d.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    float32_t4x4 uvTransform;
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // UV に行列をかける
    float2 uv = mul(float4(input.texcoord, 0, 1), gMaterial.uvTransform).xy;
    float4 texColor = gTexture.Sample(gSampler, uv);
    
    // ベースカラー
    float3 base = gMaterial.color.rgb * texColor.rgb;
    float alpha = gMaterial.color.a * texColor.a;

    if (gMaterial.enableLighting != 0)
    {
        // 法線とライト方向は正規化して dot
        float NdotL = saturate(dot(normalize(input.normal), -normalize(gDirectionalLight.direction)));
        // 半陰影：0→0.5→1.0 の範囲を [0→1] にリマップ
        float halfLambert = NdotL * 0.5f + 0.5f;
        float diff = saturate(dot(normalize(input.normal),
                         -normalize(gDirectionalLight.direction)));

        // 最低光（アンビエント）をちょい足し
        const float ambient = 0.1f;
        float3 lighting = ambient + diff * gDirectionalLight.intensity * gDirectionalLight.color.rgb;

        output.color = float4(base * lighting, alpha);
    }
    else
    {
        output.color = float4(base, alpha);
    }

    return output;
}
