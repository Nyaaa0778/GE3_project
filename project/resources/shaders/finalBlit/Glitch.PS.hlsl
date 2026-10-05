#include "FinalBlit.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct GlitchParams
{
    float32_t time;
    float32_t glitchIntensity;       // グリッチ全体の強度 (0.0 ~ 1.0)
    float32_t chromaticAberration; // 色収差 (RGBずらし) の基本強度
    float32_t scanlineIntensity;   // 走査線の強度 (0.0 ~ 1.0)

    float32_t glitchSpeed;         // リズム・更新速度 (1秒あたりのステップ数, デフォルト 12.0)
    float32_t glitchFrequency;     // 発生頻度・確率 (0.0 ~ 1.0, デフォルト 0.35)
    float32_t blockCount;          // 画面縦の分割数 (デフォルト 35.0)
    float32_t shiftScale;          // 横ズレの振れ幅倍率 (デフォルト 1.0)
};
ConstantBuffer<GlitchParams> gParams : register(b0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

// 疑似乱数関数
float rand(float2 n)
{
    return frac(sin(dot(n, float2(12.9898, 78.233))) * 43758.5453);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float2 uv = input.texcoord;
    float t = gParams.time;
    
    // 1. ブロックグリッチ (横帯ごとのUVずれ)
    // 時間によって間欠的にグリッチの強弱をつける (glitchSpeedでリズム調整)
    float speed = max(gParams.glitchSpeed, 0.1);
    float glitchTime = floor(t * speed);

    // 発生頻度 (glitchFrequency: 0.0で発生なし、1.0で常時発生)
    float pulseThreshold = clamp(1.0 - gParams.glitchFrequency, 0.0, 1.0);
    float glitchPulse = step(pulseThreshold, rand(float2(glitchTime, 1.23)));
    
    // 分割数 (blockCount)
    float count = max(gParams.blockCount, 1.0);
    float blockY = floor(uv.y * count);
    float blockRand = rand(float2(blockY, glitchTime));
    
    float shiftScale = max(gParams.shiftScale, 0.0);
    float shift = 0.0;
    if (blockRand > 0.88)
    {
        // 激しいラインズレ
        shift = (rand(float2(blockY, t * 60.0)) - 0.5) * 0.07 * gParams.glitchIntensity * shiftScale;
    }
    else if (glitchPulse > 0.5 && blockRand > 0.65)
    {
        // パルス発生時の中程度のズレ
        shift = (rand(float2(blockY, t * 30.0)) - 0.5) * 0.03 * gParams.glitchIntensity * shiftScale;
    }
    
    float2 shiftedUV = uv;
    shiftedUV.x = clamp(shiftedUV.x + shift, 0.0, 1.0);
    
    // 2. 色収差 (Chromatic Aberration: 赤と青を逆方向にずらす)
    float aberration = gParams.chromaticAberration * (1.0 + abs(shift) * 15.0);
    
    float r = gTexture.Sample(gSampler, float2(clamp(shiftedUV.x + aberration, 0.0, 1.0), shiftedUV.y)).r;
    float g = gTexture.Sample(gSampler, shiftedUV).g;
    float b = gTexture.Sample(gSampler, float2(clamp(shiftedUV.x - aberration, 0.0, 1.0), shiftedUV.y)).b;
    float a = gTexture.Sample(gSampler, shiftedUV).a;
    
    float3 col = float3(r, g, b);
    
    // 3. スキャンライン (ブラウン管・サイバーディスプレイ風の横線)
    float scanline = sin(uv.y * 700.0) * 0.5 + 0.5;
    col -= col * (scanline * gParams.scanlineIntensity);
    
    // 4. 微小粒子グレイン (アナログ・ノイズ感)
    float grain = (rand(uv + float2(t * 10.0, t * 10.0)) - 0.5) * 0.03 * gParams.glitchIntensity;
    col += grain;
    
    output.color = float32_t4(col, a);
    return output;
}
