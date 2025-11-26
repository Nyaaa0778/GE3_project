#include "Object3d.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    float4x4 uvTransform;
};
ConstantBuffer<Material> gMaterial : register(b0);

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

struct DirectionalLight
{
    float4 color; //色
    float3 direction; // 光の向き
    float intensity;
};
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput o;

    // ==== UV 変換 ====
    float2 uv = mul(gMaterial.uvTransform, float4(input.texcoord, 0, 1)).xy;

    float4 texColor = gTexture.Sample(gSampler, uv);

    float3 base = gMaterial.color.rgb * texColor.rgb;
    float alpha = gMaterial.color.a * texColor.a;
    
    if (texColor.a == 0.0)
    {
        discard;
    }
    
    if (texColor.a <= 0.5)
    {
        discard;
    }

    if (gMaterial.enableLighting != 0)
    {
        float3 N = normalize(input.normal);
        float3 L = normalize(-gDirectionalLight.direction);

        float lambert = saturate(dot(N, L)); // 拡散
        float ambient = 0.1f;

        float3 lightRGB = gDirectionalLight.color.rgb;
        float I = gDirectionalLight.intensity;

        float3 lighting = ambient + lambert * I * lightRGB;

        o.color = float4(base * lighting, alpha);
    }
    else
    {
        o.color = float4(base, alpha);
    }
    
    if (o.color.a == 0.0)
    {
        discard;
    }

    return o;
}
