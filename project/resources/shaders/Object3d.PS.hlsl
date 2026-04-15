#include"Object3d.hlsli"

struct Material
{
    float32_t4 color;
    int32_t lightingType;
    float32_t4x4 uvTransform;
    float32_t shininess;
};

ConstantBuffer<Material> gMaterial : register(b0);

struct DirectionalLight
{
    float32_t4 color; // ライトの色
    float32_t3 direction; // ライトの向き
    float intensity; // 輝度
};

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

struct PointLight
{
    float32_t4 color; // ライトの色
    float32_t3 position; // ライトの位置
    float intensity; // 輝度
};

ConstantBuffer<PointLight> gPointLight : register(b3);

struct Camera
{
    float32_t3 worldPosition;
};

ConstantBuffer<Camera> gCamera : register(b2);

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    if (gMaterial.lightingType != 0)
    {
        float3 N = normalize(input.normal);
        
        // ==========================================
        // 1. 入射光の方向ベクトルを計算
        // ==========================================
        float3 dirLightDir = normalize(gDirectionalLight.direction);
        float3 pointLightDir = normalize(input.worldPosition - gPointLight.position);
        
        // 内積 (光のやってくる方向の逆ベクトルとの内積)
        float nDotLDir = dot(N, -dirLightDir);
        float nDotLPoint = dot(N, -pointLightDir);

        // ==========================================
        // 2. 変数の準備
        // ==========================================
        float cosDir = 0.0f;
        float cosPoint = 0.0f;
        float3 specularDir = float3(0.0f, 0.0f, 0.0f);
        float3 specularPoint = float3(0.0f, 0.0f, 0.0f);

        // カメラへの方向
        float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);

        // ==========================================
        // 3. ライティングの種類ごとに計算
        // ==========================================
        if (gMaterial.lightingType == 1) // ランバート
        {
            cosDir = saturate(nDotLDir);
            cosPoint = saturate(nDotLPoint);
        }
        else if (gMaterial.lightingType == 2) // ハーフランバート
        {
            cosDir = pow(saturate(nDotLDir * 0.5f + 0.5f), 2.0f);
            cosPoint = pow(saturate(nDotLPoint * 0.5f + 0.5f), 2.0f);
        }
        else if (gMaterial.lightingType == 3) // フォン
        {
            cosDir = pow(saturate(nDotLDir * 0.5f + 0.5f), 2.0f);
            cosPoint = pow(saturate(nDotLPoint * 0.5f + 0.5f), 2.0f);

            // 平行光源の鏡面反射
            float3 reflectDir = reflect(dirLightDir, N);
            float rDotEDir = dot(reflectDir, toEye);
            specularDir = gDirectionalLight.color.rgb * gDirectionalLight.intensity * pow(saturate(rDotEDir), gMaterial.shininess);

            // 点光源の鏡面反射
            float3 reflectPoint = reflect(pointLightDir, N);
            float rDotEPoint = dot(reflectPoint, toEye);
            specularPoint = gPointLight.color.rgb * gPointLight.intensity * pow(saturate(rDotEPoint), gMaterial.shininess);
        }
        else if (gMaterial.lightingType == 4) // ブリン・フォン
        {
            cosDir = pow(saturate(nDotLDir * 0.5f + 0.5f), 2.0f);
            cosPoint = pow(saturate(nDotLPoint * 0.5f + 0.5f), 2.0f);

            // 平行光源の鏡面反射
            float3 halfDir = normalize(-dirLightDir + toEye);
            float nDotHDir = dot(N, halfDir);
            specularDir = gDirectionalLight.color.rgb * gDirectionalLight.intensity * pow(saturate(nDotHDir), gMaterial.shininess);

            // 点光源の鏡面反射
            float3 halfPoint = normalize(-pointLightDir + toEye);
            float nDotHPoint = dot(N, halfPoint);
            specularPoint = gPointLight.color.rgb * gPointLight.intensity * pow(saturate(nDotHPoint), gMaterial.shininess);
        }

        // ==========================================
        // 4. 拡散反射 (Diffuse) の計算
        // ==========================================
        float3 baseColor = gMaterial.color.rgb * textureColor.rgb;

        float3 diffuseDir = baseColor * gDirectionalLight.color.rgb * cosDir * gDirectionalLight.intensity;
        float3 diffusePoint = baseColor * gPointLight.color.rgb * cosPoint * gPointLight.intensity;

        // ==========================================
        // 5. 全部足す
        // ==========================================
        output.color.rgb = diffuseDir + specularDir + diffusePoint + specularPoint;
        
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
 
    return output;
}