#include"Sprite.hlsli"

struct TransformationMatrix
{
    float32_t4x4 WVP; // 正射影でも透視でもOK。CPU側で用意した行列を突っ込む
};
ConstantBuffer<TransformationMatrix> gTransform : register(b0);

// 頂点レイアウトは「position + texcoord」を想定
struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    // クリップ空間に変換
    output.position = mul(input.position, gTransform.WVP);

    // UV はそのまま流す
    output.texcoord = input.texcoord;

    return output;
}