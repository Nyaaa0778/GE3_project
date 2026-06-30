#include "FinalBlit.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct RandomNoiseParams {
    float32_t time;
};
ConstantBuffer<RandomNoiseParams> gParams : register(b0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

// 2D座標から1Dの擬似乱数を生成する関数
float32_t rand2dTo1d(float32_t2 uv)
{
    return frac(sin(dot(uv, float32_t2(12.9898f, 78.233f))) * 43758.5453f);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // 乱数生成。引数にtexcoord + 時間を渡している
    float32_t random = rand2dTo1d(input.texcoord + float32_t2(gParams.time, gParams.time));
    
    // 色にする
    output.color = float32_t4(random, random, random, 1.0f);
    
    return output;
}
