#include"Object3d.hlsli"

// ----- マテリアル -----
struct Material
{
    float32_t4 color;
    int32_t lightingType;
    float32_t4x4 uvTransform;
    float32_t shininess;
    float32_t environmentCoefficient;
};
ConstantBuffer<Material> gMaterial : register(b0);

// ----- ライトデータ -----
struct DirectionalLight
{
    float32_t4 color; // ライトの色
    float32_t3 direction; // ライトの向き
    float32_t intensity; // 輝度
};

struct PointLight
{
    float32_t4 color;
    float32_t3 position;
    float32_t intensity;
    float32_t distance;
    float32_t decay;
    int32_t enabled;
    float32_t pad;
};

struct SpotLight
{
    float32_t4 color;
    float32_t3 position;
    float32_t intensity;
    float32_t3 direction;
    float32_t distance;
    float32_t decay;
    float32_t cosAngle;
    float32_t cosFalloffStart;
    int32_t enabled;
};

struct LightData
{
    DirectionalLight directionalLight;
    PointLight pointLights[8];
    SpotLight spotLights[4];
};

ConstantBuffer<LightData> gLightData : register(b1);

// ----- カメラ -----
struct Camera
{
    float32_t3 worldPosition;
};
ConstantBuffer<Camera> gCamera : register(b2);

Texture2D<float32_t4> gTexture : register(t0);
TextureCube<float32_t4> gEnvironmentTexture : register(t1);

SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

// ==========================================
// DirectionalLightの計算
// ==========================================
float3 CalculateDirectionalLight(float3 normal, float3 toEye, float3 baseColor)
{
    float3 lightDir = normalize(gLightData.directionalLight.direction);
    float3 diffuse = float3(0.0f, 0.0f, 0.0f);
    float3 specular = float3(0.0f, 0.0f, 0.0f);

    /* -------------------------------------------
      ① 拡散反射 (Diffuse) の計算
    ------------------------------------------- */
    if (gMaterial.lightingType == 1 || gMaterial.lightingType == 3 || gMaterial.lightingType == 4)
    {
        // ランバート (1:Lambert, 3:Phong, 4:BlinnPhong は基本ランバートベース)
        float cos = saturate(dot(normal, -lightDir));
        diffuse = baseColor * gLightData.directionalLight.color.rgb * cos * gLightData.directionalLight.intensity;
    }
    else if (gMaterial.lightingType == 2)
    {
        // ハーフランバート (2:HalfLambert)
        float cos = pow(saturate(dot(normal, -lightDir) * 0.5f + 0.5f), 2.0f);
        diffuse = baseColor * gLightData.directionalLight.color.rgb * cos * gLightData.directionalLight.intensity;
    }

    /* -------------------------------------------
      ② 鏡面反射 (Specular) の計算
    ------------------------------------------- */
    if (gMaterial.lightingType == 3)
    {
        // フォン反射 (3:Phong)
        float3 reflectDir = reflect(lightDir, normal);
        float rDotE = dot(reflectDir, toEye);
        specular = gLightData.directionalLight.color.rgb * gLightData.directionalLight.intensity * pow(saturate(rDotE), gMaterial.shininess);
    }
    else if (gMaterial.lightingType == 4)
    {
        // ブリン・フォン反射 (4:BlinnPhong)
        float3 halfVector = normalize(-lightDir + toEye);
        float nDotH = dot(normal, halfVector);
        specular = gLightData.directionalLight.color.rgb * gLightData.directionalLight.intensity * pow(saturate(nDotH), gMaterial.shininess);
    }

    // 拡散反射と鏡面反射を足して返す
    return diffuse + specular;
}

// ==========================================
// PointLightの計算
// ==========================================
float3 CalculatePointLight(PointLight light, float3 normal, float3 worldPosition, float3 toEye, float3 baseColor)
{
    // ライトへの方向と距離
    float3 lightVec = worldPosition - light.position;
    float3 lightDir = normalize(lightVec);
    float distance = length(lightVec);
    
    // 1. 距離による減衰 (Factor)
    float factor = pow(saturate(-distance / light.distance + 1.0f), light.decay);
    
    // 2. 拡散反射 (Half-Lambert)
    float cos = pow(saturate(dot(normal, -lightDir) * 0.5f + 0.5f), 2.0f);
    float3 diffuse = baseColor * light.color.rgb * cos * light.intensity * factor;
    
    // 3. 鏡面反射 (Blinn-Phong)
    float3 halfVector = normalize(-lightDir + toEye);
    float nDotH = dot(normal, halfVector);
    float3 specular = light.color.rgb * light.intensity * pow(saturate(nDotH), gMaterial.shininess) * factor;
    
    return diffuse + specular;
}

// ==========================================
// SpotLightの計算
// ==========================================
float3 CalculateSpotLight(SpotLight light, float3 normal, float3 worldPosition, float3 toEye, float3 baseColor)
{
    // 基本的なベクトル計算
    float3 lightVec = worldPosition - light.position;
    float3 lightDir = normalize(lightVec);
    float distance = length(lightVec);
    
    // 1. 距離による減衰
    float factor = pow(saturate(-distance / light.distance + 1.0f), light.decay);
    
    // 2. 角度による減衰 (Spotlight Falloff)
    float cosAngle = dot(lightDir, normalize(light.direction));
    float falloffFactor = saturate((cosAngle - light.cosAngle) / (light.cosFalloffStart - light.cosAngle));
    
    // 3. 拡散反射 (Half-Lambert)
    float cos = pow(saturate(dot(normal, -lightDir) * 0.5f + 0.5f), 2.0f);
    float3 diffuse = baseColor * light.color.rgb * cos * light.intensity * factor * falloffFactor;
    
    // 4. 鏡面反射 (Blinn-Phong)
    float3 halfVector = normalize(-lightDir + toEye);
    float nDotH = dot(normal, halfVector);
    float3 specular = light.color.rgb * light.intensity * pow(saturate(nDotH), gMaterial.shininess) * factor * falloffFactor;
    
    return diffuse + specular;
}

// ==========================================
// メイン関数
// ==========================================
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // 1. テクスチャのサンプリングとベースカラー
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    float3 baseColor = gMaterial.color.rgb * textureColor.rgb;

    // 2. 共通で使うベクトルの計算
    float3 N = normalize(input.normal);
    float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);

    // 3. 最終的な色を保存する変数（最初は真っ黒）
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);

    // ----------------------------------------------------
    // ライトの計算と合成（足し算）
    // ----------------------------------------------------
    if (gMaterial.lightingType != 0)
    {
        // ① 平行光源の計算
        finalColor += CalculateDirectionalLight(N, toEye, baseColor);

        // ② 点光源の配列ループ
        for (int32_t i = 0; i < 8; ++i)
        {
            if (gLightData.pointLights[i].enabled != 0)
            {
                finalColor += CalculatePointLight(gLightData.pointLights[i], N, input.worldPosition, toEye, baseColor);
            }
        }

        // ③ スポットライトの配列ループ
        for (int32_t j = 0; j < 4; ++j)
        {
            if (gLightData.spotLights[j].enabled != 0)
            {
                finalColor += CalculateSpotLight(gLightData.spotLights[j], N, input.worldPosition, toEye, baseColor);
            }
        }
    }
    else
    {
        // ライティング無しの場合はそのままの色
        finalColor = baseColor;
    }

    // ----------------------------------------------------
    // 4. 結果を出力
    // ----------------------------------------------------
    output.color.rgb = finalColor;
    output.color.a = gMaterial.color.a * textureColor.a;
    
    // 環境マップによる環境光の計算と加算
    if (gMaterial.environmentCoefficient > 0.0f)
    {
        float3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
        float3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
        float4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectedVector);
        
        output.color.rgb += environmentColor.rgb * gMaterial.environmentCoefficient;
    }

    return output;
}