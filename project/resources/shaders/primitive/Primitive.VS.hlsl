#include "Primitive.hlsli"

struct TransformationMatrix {
    float32_t4x4 WVP;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);

struct Material {
    float32_t4 color;
    float32_t4x4 uvTransform;
};
ConstantBuffer<Material> gMaterial : register(b0);

struct VertexShaderInput {
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input) {
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.texcoord = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform).xy;
    output.color = gMaterial.color;
    return output;
}
