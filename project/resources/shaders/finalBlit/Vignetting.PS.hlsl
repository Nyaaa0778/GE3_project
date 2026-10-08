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
    
    // 画面中心(0,0)から外枠(1,1)への正規化座標
    float2 uv = abs(input.texcoord - 0.5f) * 2.0f;
    
    // 画面の最外周（端の約2.5%の極細フチ枠）のみに限定し、内側95%以上は完全に元の画面を維持
    float2 d = saturate((uv - 0.95f) / 0.05f);
    float border = smoothstep(0.0f, 1.0f, max(d.x, d.y));
    
    // 端枠がじゃっかん赤くなる程度に抑えて補間 (最大約40%の赤みブレンド)
    float blendRate = border * 0.40f;
    output.color.rgb = lerp(output.color.rgb, gVignetteParams.color.rgb, blendRate);
    
    return output;
}