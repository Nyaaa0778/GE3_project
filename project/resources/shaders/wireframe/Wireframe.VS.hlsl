#include "Wireframe.hlsli"

struct InstanceData
{
    float4x4 WVP;
    float4 color;
};
StructuredBuffer<InstanceData> gInstances : register(t0);

struct VertexShaderInput
{
    float4 position : POSITION0;
};

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gInstances[instanceId].WVP);
    output.depth = max(output.position.w, 0.0f);
    output.color = gInstances[instanceId].color;
    return output;
}

