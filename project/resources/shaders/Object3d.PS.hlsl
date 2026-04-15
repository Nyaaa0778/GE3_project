#include"Object3d.hlsli"

// ----- マテリアル -----
struct Material
{
    float32_t4 color;
    int32_t lightingType;
    float32_t4x4 uvTransform;
    float32_t shininess;
};

ConstantBuffer<Material> gMaterial : register(b0);

// ----- 平行光源 -----
struct DirectionalLight
{
    float32_t4 color; // ライトの色
    float32_t3 direction; // ライトの向き
    float32_t intensity; // 輝度
};

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

// ----- 点光源 -----
struct PointLight
{
    float32_t4 color; // ライトの色
    float32_t3 position; // ライトの位置
    float32_t intensity; // 輝度
    float32_t radius; // ライトの届く最大距離
    float32_t decay; // 減衰率
    int32_t lightType;
};

ConstantBuffer<PointLight> gPointLight : register(b3);

// ----- スポットライト -----
struct SpotLight
{
    float32_t4 color; // ライトの色
    float32_t3 position; // ライトの位置
    float32_t intensity; // 輝度
    float32_t3 direction; // スポットライトの方向
    float32_t distance; // ライトの届く最大距離
    float32_t decay; // 減衰率
    float32_t cosAngle; // スポットライトの余弦
    float32_t cosFalloffStart;
    int32_t lightType;
};

ConstantBuffer<SpotLight> gSpotLight : register(b4);

// ----- カメラ -----
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

// ==========================================
// DirectionalLightの計算
// ==========================================
float3 CalculateDirectionalLight(float3 normal, float3 toEye, float3 baseColor)
{
    float3 lightDir = normalize(gDirectionalLight.direction);
    float3 diffuse = float3(0.0f, 0.0f, 0.0f);
    float3 specular = float3(0.0f, 0.0f, 0.0f);

    /* -------------------------------------------
      ① 拡散反射 (Diffuse) の計算
    ------------------------------------------- */
    if (gMaterial.lightingType == 1 || gMaterial.lightingType == 3 || gMaterial.lightingType == 4)
    {
        // ランバート (1:Lambert, 3:Phong, 4:BlinnPhong は基本ランバートベース)
        float cos = saturate(dot(normal, -lightDir));
        diffuse = baseColor * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
    }
    else if (gMaterial.lightingType == 2)
    {
        // ハーフランバート (2:HalfLambert)
        float cos = pow(saturate(dot(normal, -lightDir) * 0.5f + 0.5f), 2.0f);
        diffuse = baseColor * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
    }

    /* -------------------------------------------
      ② 鏡面反射 (Specular) の計算
    ------------------------------------------- */
    if (gMaterial.lightingType == 3)
    {
        // フォン反射 (3:Phong)
        float3 reflectDir = reflect(lightDir, normal);
        float rDotE = dot(reflectDir, toEye);
        specular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * pow(saturate(rDotE), gMaterial.shininess);
    }
    else if (gMaterial.lightingType == 4)
    {
        // ブリン・フォン反射 (4:BlinnPhong)
        float3 halfVector = normalize(-lightDir + toEye);
        float nDotH = dot(normal, halfVector);
        specular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * pow(saturate(nDotH), gMaterial.shininess);
    }

    // 拡散反射と鏡面反射を足して返す
    return diffuse + specular;
}

// ==========================================
// PointLightの計算
// ==========================================
float3 CalculatePointLight(float3 normal, float3 worldPosition, float3 toEye, float3 baseColor)
{
    float3 lightDir = normalize(worldPosition - gPointLight.position);
    float distance = length(gPointLight.position - worldPosition);
    float factor = pow(saturate(-distance / gPointLight.radius + 1.0f), gPointLight.decay);
    
    float cos = pow(saturate(dot(normal, -lightDir) * 0.5f + 0.5f), 2.0f);
    
    // 鏡面反射
    float3 halfVector = normalize(-lightDir + toEye);
    float nDotH = dot(normal, halfVector);
    float3 specular = gPointLight.color.rgb * gPointLight.intensity * pow(saturate(nDotH), gMaterial.shininess) * factor;
    
    // 拡散反射
    float3 diffuse = baseColor * gPointLight.color.rgb * cos * gPointLight.intensity * factor;
    
    return diffuse + specular;
}

// ==========================================
// SpotLightの計算
// ==========================================
float3 CalculateSpotLight(float3 normal, float3 worldPosition, float3 toEye, float3 baseColor)
{
    // ※ ここに前回提示したスポットライトの計算処理を入れます
    float3 lightDir = normalize(worldPosition - gSpotLight.position);
    float distance = length(gSpotLight.position - worldPosition);
    float factor = pow(saturate(-distance / gSpotLight.distance + 1.0f), gSpotLight.decay);
    
    // 角度による減衰
    float cosAngle = dot(lightDir, normalize(gSpotLight.direction));
    float falloffFactor = saturate((cosAngle - gSpotLight.cosAngle) / (gSpotLight.cosFalloffStart - gSpotLight.cosAngle));
    
    float cos = pow(saturate(dot(normal, -lightDir) * 0.5f + 0.5f), 2.0f);
    
    // 鏡面反射
    float3 halfVector = normalize(-lightDir + toEye);
    float nDotH = dot(normal, halfVector);
    float3 specular = gSpotLight.color.rgb * gSpotLight.intensity * pow(saturate(nDotH), gMaterial.shininess) * factor * falloffFactor;
    
    // 拡散反射
    float3 diffuse = baseColor * gSpotLight.color.rgb * cos * gSpotLight.intensity * factor * falloffFactor;
    
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
        // ① 平行光源は常に計算して足す
        finalColor += CalculateDirectionalLight(N, toEye, baseColor);

        // ② もしC++側で設定されているのが PointLight なら
        if (gPointLight.lightType == 1)
        {
            finalColor += CalculatePointLight(N, input.worldPosition, toEye, baseColor);
        }
        // ③ もしC++側で設定されているのが SpotLight なら
        if (gSpotLight.lightType == 2)
        {
            finalColor += CalculateSpotLight(N, input.worldPosition, toEye, baseColor);
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

    return output;
}