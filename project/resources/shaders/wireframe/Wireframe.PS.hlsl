#include"Wireframe.hlsli"

struct WireframeColor
{
    float4 color;
};
ConstantBuffer<WireframeColor> gMaterial : register(b1);

float4 main() : SV_TARGET
{
    return gMaterial.color; // 例: float4(0.0f, 1.0f, 0.0f, 1.0f) 緑色など
}
