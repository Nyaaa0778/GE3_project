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

    // UV中心からの正規化距離 (0.0: 中心, 1.0: 境界)
    float2 uv = input.texcoord;
    float2 centerOffset = uv - float2(0.5, 0.5);
    float dist = length(centerOffset) * 2.0;

    // テクスチャサンプリング
    float4 texColor = gTexture.Sample(gSampler, uv);

    // 1. 中心の白熱コア (非常に明るい純白)
    float core = saturate(1.0 - dist * 2.8);
    core = pow(core, 2.5);

    // 2. 外側へのフレネル風グロー減衰 (滑らかな指数減衰)
    float glow = saturate(1.0 - dist);
    glow = pow(glow, 1.8);

    // 3. 発光色の合成: 中心は高輝度白、周囲は入力カラー
    float3 baseColor = input.color.rgb;
    float3 emissiveColor = lerp(baseColor * 1.8, float3(1.5, 1.5, 1.5), core);

    // 加算ブレンド用に出力カラーを合成
    output.color.rgb = emissiveColor * texColor.rgb * glow;
    output.color.a = texColor.a * input.color.a * glow;

    return output;
}
