#include "FinalBlit.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

static const float32_t2 kIndex3x3[3][3] =
{
    { { -1.0f, -1.0f }, { -1.0f, 0.0f }, { -1.0f, 1.0f } }, // x = 0 (左列: y=-1, y=0, y=1)
    { { 0.0f, -1.0f }, { 0.0f, 0.0f }, { 0.0f, 1.0f } }, // x = 1 (中央列: y=-1, y=0, y=1)
    { { 1.0f, -1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f } } // x = 2 (右列: y=-1, y=0, y=1)
};

static const float32_t kKernel3x3[3][3] =
{
    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f }
};

static const float32_t kPrewittHorizontalKernel[3][3] =
{
    { -1.0f / 6.0f, 0.0f, 1.0 / 6.0 },
    { -1.0f / 6.0f, 0.0f, 1.0 / 6.0 },
    { -1.0f / 6.0f, 0.0f, 1.0 / 6.0 },
};

static const float32_t kPrewittVerticalKernel[3][3] =
{
    { -1.0f / 6.0f, -1.0f / 6.0f, 1.0f / 6.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f },
};

float32_t Luminance(float32_t3 v)
{
    return dot(v, float32_t3(0.2125f, 0.7154f, 0.0721f));

}

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    uint32_t width;
    uint32_t height;
    
    gTexture.GetDimensions(width, height);
    
    float32_t2 uvStepSize = float32_t2(rcp(width), rcp(height));
    
    PixelShaderOutput output;
    output.color.rgb = float32_t3(0.0f, 0.0f, 0.0f);
    output.color.a = 1.0f;
    
    float32_t2 difference = float32_t2(0.0f, 0.0f);
    
    for (int32_t x = 0; x < 3; ++x)
    {
        for (int32_t y = 0; y < 3; ++y)
        {
            float32_t2 texcoord = input.texcoord + kIndex3x3[x][y] * uvStepSize;
            float32_t3 fetchColor = gTexture.Sample(gSampler, texcoord).rgb;
            float32_t luminance = Luminance(fetchColor);
            difference.x += luminance * kPrewittHorizontalKernel[x][y];
            difference.y += luminance * kPrewittVerticalKernel[x][y];
        }

    }
    
    // 変化の長さをウェイトとして合成
    float32_t weight = length(difference);
    weight = saturate(weight);
    output.color.rgb = weight;
    output.color.a = 1.0f;
    
    return output;
}