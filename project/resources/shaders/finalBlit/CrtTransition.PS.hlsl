#include "FinalBlit.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct CrtTransitionParams
{
    float32_t time;
    float32_t glitchIntensity;       // RGBグリッチ強度 (0.0 ~ 1.0)
    float32_t chromaticAberration;   // 色収差・RGBズレ量 (0.0 ~ 0.1)
    float32_t scanlineIntensity;     // 走査線強度 (0.0 ~ 1.0)

    float32_t collapseX;             // 水平スケール (1.0 = 全幅, 0.0 = 収縮)
    float32_t collapseY;             // 垂直スケール (1.0 = 全高, 0.0 = 収縮)
    float32_t flashIntensity;        // 白熱・発光強度 (0.0 ~ 2.0)
    float32_t screenCurvature;       // ブラウン管の湾曲歪み (0.0 ~ 0.1)
};
ConstantBuffer<CrtTransitionParams> gParams : register(b0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

// 疑似乱数
float rand(float2 n)
{
    return frac(sin(dot(n, float2(12.9898, 78.233))) * 43758.5453);
}

// ブラウン管の樽型歪み (Barrel Distortion)
float2 ApplyCrtCurvature(float2 uv, float curvature)
{
    if (curvature <= 0.001)
    {
        return uv;
    }
    float2 centered = uv - 0.5;
    float r2 = dot(centered, centered);
    float2 distorted = centered * (1.0 + curvature * r2 * 2.0);
    return distorted + 0.5;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float2 uv = input.texcoord;
    float t = gParams.time;

    // 0. 完全に消滅（真っ暗）している場合の早期リターン
    if (gParams.collapseX <= 0.0001 && gParams.collapseY <= 0.0001 && gParams.flashIntensity <= 0.001)
    {
        output.color = float32_t4(0.0, 0.0, 0.0, 1.0);
        return output;
    }

    // 1. CRT樽型湾曲の適用
    float2 crtUV = ApplyCrtCurvature(uv, gParams.screenCurvature);

    // 2. 画面収縮（Collapse）計算
    // 中心(0.5, 0.5)を基準に拡大・縮小
    float2 centeredUV = crtUV - 0.5;
    float safeScaleX = max(gParams.collapseX, 0.0002);
    float safeScaleY = max(gParams.collapseY, 0.0002);

    float2 scaledUV;
    scaledUV.x = (centeredUV.x / safeScaleX) + 0.5;
    scaledUV.y = (centeredUV.y / safeScaleY) + 0.5;

    // 表示領域の判定
    bool isInside = (scaledUV.x >= 0.0 && scaledUV.x <= 1.0 && scaledUV.y >= 0.0 && scaledUV.y <= 1.0);

    float3 col = float3(0.0, 0.0, 0.0);

    // 3. 画面内描画（テクスチャサンプリング ＋ RGBグリッチ）
    if (isInside && gParams.collapseX > 0.0005 && gParams.collapseY > 0.0005)
    {
        float2 sampleUV = scaledUV;
        float aberration = gParams.chromaticAberration;

        // グリッチ発生時の横帯ノイズ（Block Glitch）
        if (gParams.glitchIntensity > 0.001)
        {
            float blockCount = 30.0;
            float blockY = floor(sampleUV.y * blockCount);
            float blockNoise = rand(float2(blockY, floor(t * 35.0)));
            
            if (blockNoise > 0.45)
            {
                float shift = (rand(float2(blockY, t * 75.0)) - 0.5) * 0.10 * gParams.glitchIntensity;
                sampleUV.x = clamp(sampleUV.x + shift, 0.0, 1.0);
                aberration += abs(shift) * 1.5;
            }
        }

        // RGB色収差（Chromatic Aberration）サンプリング
        float r = gTexture.Sample(gSampler, float2(clamp(sampleUV.x + aberration, 0.0, 1.0), sampleUV.y)).r;
        float g = gTexture.Sample(gSampler, sampleUV).g;
        float b = gTexture.Sample(gSampler, float2(clamp(sampleUV.x - aberration, 0.0, 1.0), sampleUV.y)).b;
        col = float3(r, g, b);

        // 走査線（スキャンライン）
        float scanline = sin(crtUV.y * 800.0) * 0.5 + 0.5;
        col -= col * (scanline * gParams.scanlineIntensity);

        // グリッチ粒状ノイズ
        if (gParams.glitchIntensity > 0.001)
        {
            float grain = (rand(sampleUV + t * 5.0) - 0.5) * 0.12 * gParams.glitchIntensity;
            col += grain;
        }

        // 収縮による画面内白熱（白飛び）
        if (gParams.flashIntensity > 0.01)
        {
            col += gParams.flashIntensity * float3(1.3, 1.3, 1.4);
        }
    }

    // 4. ブラウン管特有の白熱グロー（水平線・中央輝点のにじみ・光芒）
    if (gParams.flashIntensity > 0.01)
    {
        // Y方向（水平線の太さに応じたガウシアン風減衰）
        float distY = abs(centeredUV.y);
        float lineSpreadY = max(gParams.collapseY * 0.5, 0.0015);
        float glowY = exp(-distY * distY / (2.0 * lineSpreadY * lineSpreadY));

        // X方向（水平ラインの長さに応じた範囲）
        float distX = abs(centeredUV.x);
        float lineLengthX = max(gParams.collapseX * 0.5, 0.0015);
        float glowX = 0.0;

        if (distX <= lineLengthX)
        {
            glowX = 1.0;
        }
        else
        {
            float extraX = distX - lineLengthX;
            glowX = exp(-extraX * 120.0);
        }

        // 白熱ラインの光芒
        float lineGlow = glowY * glowX * gParams.flashIntensity;
        col += float3(lineGlow, lineGlow, lineGlow * 1.15);

        // 中心の輝点（ドット）の強烈な残光
        float dotDist = length(centeredUV);
        float dotGlow = exp(-dotDist * 180.0) * gParams.flashIntensity * 1.5;
        col += float3(dotGlow, dotGlow, dotGlow);
    }

    // 画面枠の丸みフェード（CRTのベゼル縁）
    if (gParams.screenCurvature > 0.001)
    {
        float2 vignetteUV = crtUV * (1.0 - crtUV.yx);
        float vignette = vignetteUV.x * vignetteUV.y * 35.0;
        vignette = clamp(pow(vignette, 0.25), 0.0, 1.0);
        col *= vignette;
    }

    output.color = float32_t4(col, 1.0);
    return output;
}
