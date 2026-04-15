// LightManager.cpp
#include "LightManager.h"
#include "DirectXCommon.h"

// シングルトンのインスタンス取得
LightManager* LightManager::GetInstance() {
    static LightManager instance;
    return &instance;
}

// 初期化
void LightManager::Initialize(DirectXCommon* dxCommon) {
    // --- Directional Light ---
    // 定数バッファ（GPUメモリ）の作成
    directionalLightBuffer_ = dxCommon->CreateBufferResource(sizeof(DirectionalLight));
    // マッピングしてC++から書き込めるようにする
    directionalLightBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));

    // デフォルトのライト設定（白くて、斜め下を向いている光）
    directionalLightData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
    directionalLightData_->direction = {0.0f, -1.0f, 0.5f};
    directionalLightData_->intensity = 1.5f;

    // --- Point Light (ここを追加・修正) ---
    // PointLight用のバッファを作成し、マップする
    pointLightBuffer_ = dxCommon->CreateBufferResource(sizeof(PointLight));
    pointLightBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData_));

    pointLightData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
    pointLightData_->position = {0.0f, -1.0f, 0.5f};
    pointLightData_->intensity = 1.5f;
}

// 定数バッファへの転送（今回はMapしっぱなしなので空でOK）
void LightManager::TransferData() {
}