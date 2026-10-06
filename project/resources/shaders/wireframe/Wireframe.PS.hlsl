#include "Wireframe.hlsli"

struct WireframeMaterial
{
    float4 color;
    float fogNear;
    float fogFar;
    float2 pad;
};
ConstantBuffer<WireframeMaterial> gMaterial : register(b1);

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float4 finalColor = gMaterial.color;

    // 距離フォグ / デプスフェード (奥にいくほど徐々に見えなくなる)
    if (gMaterial.fogFar > gMaterial.fogNear && gMaterial.fogFar > 0.0f)
    {
        // ビュー深度に基づき、手前(1.0)から奥(0.0)へスムーズに減衰
        float fogFactor = saturate((gMaterial.fogFar - input.depth) / (gMaterial.fogFar - gMaterial.fogNear));
        // スムーズな減衰 (2乗カーブでより自然に消滅)
        fogFactor = fogFactor * fogFactor;

        finalColor.a *= fogFactor;
        finalColor.rgb *= fogFactor;

        // 完全に透明になったピクセルは破棄
        if (finalColor.a <= 0.001f)
        {
            discard;
        }
    }

    return finalColor;
}

