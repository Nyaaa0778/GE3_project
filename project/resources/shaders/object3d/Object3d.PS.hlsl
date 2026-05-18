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

// ----- 平行光源 -----
struct DirectionalLight
{
    float32_t4 color; // ライトの色
    float32_t3 direction; // ライトの向き
    float32_t intensity; // 輝度
};
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

// ----- ローカルライト (Point & Spot 統合) -----
struct LocalLight
{
    float32_t4 color;
    float32_t3 position;
    float32_t intensity;
    float32_t3 direction;
    float32_t distance;
    float32_t decay;
    float32_t cosAngle;
    float32_t cosFalloffStart;
    int32_t type; // 0: Point, 1: Spot
};
ConstantBuffer<LocalLight> gLocalLight : register(b3);

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
    float3 lightDir = normalize(gDirectionalLight.direction);
    float3 diffuse = float3(0.0f, 0.0f, 0.0f);
    float3 specular = float3(0.0f, 0.0f, 0.0f);

    /* -------------------------------------------
      ① 拡散反射 (Diffuse) の計算
    ------------------------------------------- */
    // ハーフランバートを適用するタイプ： 2:HalfLambert, 3:Phong, 4:BlinnPhong
    if (gMaterial.lightingType == 2 || gMaterial.lightingType == 3 || gMaterial.lightingType == 4)
    {
        float cos = pow(saturate(dot(normal, -lightDir) * 0.5f + 0.5f), 2.0f);
        diffuse = baseColor * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
    }
    // 通常のランバートを適用するタイプ： 1:Lambert
    else if (gMaterial.lightingType == 1)
    {
        float cos = saturate(dot(normal, -lightDir));
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
    // ライトへの方向と距離
    float3 lightVec = worldPosition - gLocalLight.position;
    float3 lightDir = normalize(lightVec);
    float distance = length(lightVec);
    
    // 1. 距離による減衰 (Factor)
    // gLocalLight.distance は有効半径として扱う
    float factor = pow(saturate(-distance / gLocalLight.distance + 1.0f), gLocalLight.decay);
    
    // 2. 拡散反射 (Half-Lambert)
    float cos = pow(saturate(dot(normal, -lightDir) * 0.5f + 0.5f), 2.0f);
    float3 diffuse = baseColor * gLocalLight.color.rgb * cos * gLocalLight.intensity * factor;
    
    // 3. 鏡面反射 (Blinn-Phong)
    float3 halfVector = normalize(-lightDir + toEye);
    float nDotH = dot(normal, halfVector);
    float3 specular = gLocalLight.color.rgb * gLocalLight.intensity * pow(saturate(nDotH), gMaterial.shininess) * factor;
    
    return diffuse + specular;
}

// ==========================================
// SpotLightの計算
// ==========================================
float3 CalculateSpotLight(float3 normal, float3 worldPosition, float3 toEye, float3 baseColor)
{
    // 基本的なベクトル計算
    float3 lightVec = worldPosition - gLocalLight.position;
    float3 lightDir = normalize(lightVec);
    float distance = length(lightVec);
    
    // 1. 距離による減衰
    float factor = pow(saturate(-distance / gLocalLight.distance + 1.0f), gLocalLight.decay);
    
    // 2. 角度による減衰 (Spotlight Falloff)
    // ライトの向きと、ピクセルへの方向の余弦（cos）を求める
    float cosAngle = dot(lightDir, normalize(gLocalLight.direction));
    // 指定された角度範囲で線形補間して減衰させる
    float falloffFactor = saturate((cosAngle - gLocalLight.cosAngle) / (gLocalLight.cosFalloffStart - gLocalLight.cosAngle));
    
    // 3. 拡散反射 (Half-Lambert)
    float cos = pow(saturate(dot(normal, -lightDir) * 0.5f + 0.5f), 2.0f);
    float3 diffuse = baseColor * gLocalLight.color.rgb * cos * gLocalLight.intensity * factor * falloffFactor;
    
    // 4. 鏡面反射 (Blinn-Phong)
    float3 halfVector = normalize(-lightDir + toEye);
    float nDotH = dot(normal, halfVector);
    float3 specular = gLocalLight.color.rgb * gLocalLight.intensity * pow(saturate(nDotH), gMaterial.shininess) * factor * falloffFactor;
    
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

        // ② ローカルライトの計算 (C++側から送られてきた type で分岐)
        if (gLocalLight.type == 0)
        {
            // PointLightとして計算 (必要な引数はgLocalLightから渡すように引数を調整してください)
            finalColor += CalculatePointLight(N, input.worldPosition, toEye, baseColor);
        }
        else if (gLocalLight.type == 1)
        {
            // SpotLightとして計算
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
    
    // 環境マップによる環境光の計算と加算
    float3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
    float3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
    float4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectedVector);
    
    output.color.rgb += environmentColor.rgb * gMaterial.environmentCoefficient;

    return output;
}