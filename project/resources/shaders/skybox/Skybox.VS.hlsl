#include "Skybox.hlsli"

struct TransformationMatrix
{
    float32_t4x4 WVP;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    output.position = mul(float32_t4(input.position.xyz, 0.0f), gTransformationMatrix.WVP);
    output.position = output.position.xyww;
    output.texcoord = input.position.xyz;
    
    return output;
}