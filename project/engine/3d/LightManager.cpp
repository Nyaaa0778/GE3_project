#define NOMINMAX
#include "LightManager.h"

#include "MathUtility.h"
#include "DirectXCommon.h"

#include <numbers>
#include <algorithm>
#include <cmath>

using namespace MathUtility;

//================================================================================
// シングルトンのインスタンス取得
//================================================================================
LightManager* LightManager::GetInstance() {
	static LightManager instance;
	return &instance;
}

//================================================================================
// 初期化
//================================================================================
void LightManager::Initialize(DirectXCommon* dxCommon) {
	// 定数バッファ（GPUメモリ）の作成
	lightDataBuffer_ = dxCommon->CreateBufferResource(sizeof(LightData));

	// マッピングしてC++から書き込めるようにする
	lightDataBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&lightData_));

	// 1. DirectionalLight のデフォルト設定 (白光、斜め下、輝度0.0f)
	lightData_->directionalLight.color = {1.0f, 1.0f, 1.0f, 1.0f};
	lightData_->directionalLight.direction = {0.0f, -1.0f, 0.5f};
	lightData_->directionalLight.intensity = 0.0f;

	// 2. PointLight のデフォルト設定 (全て無効)
	for (int i = 0; i < kMaxPointLights; ++i) {
		lightData_->pointLights[i].color = {1.0f, 1.0f, 1.0f, 1.0f};
		lightData_->pointLights[i].position = {0.0f, 0.0f, 0.0f};
		lightData_->pointLights[i].intensity = 1.0f;
		lightData_->pointLights[i].distance = 10.0f;
		lightData_->pointLights[i].decay = 1.0f;
		lightData_->pointLights[i].enabled = 0; // 無効化
		lightData_->pointLights[i].pad = 0.0f;
	}

	// 3. SpotLight のデフォルト設定 (全て無効)
	for (int i = 0; i < kMaxSpotLights; ++i) {
		lightData_->spotLights[i].color = {1.0f, 1.0f, 1.0f, 1.0f};
		lightData_->spotLights[i].position = {0.0f, 0.0f, 0.0f};
		lightData_->spotLights[i].intensity = 1.0f;
		lightData_->spotLights[i].direction = {0.0f, -1.0f, 0.0f};
		lightData_->spotLights[i].distance = 10.0f;
		lightData_->spotLights[i].decay = 1.0f;
		lightData_->spotLights[i].cosAngle = std::cos(std::numbers::pi_v<float> / 4.0f);
		lightData_->spotLights[i].cosFalloffStart = 1.0f;
		lightData_->spotLights[i].enabled = 0; // 無効化
	}
}