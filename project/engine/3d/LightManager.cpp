#define NOMINMAX
#include "LightManager.h"

#include "MathUtility.h"
#include "DirectXCommon.h"

#include <numbers>
#include <algorithm>

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
	// 各ライトの初期化関数を呼び出し、肥大化を防ぐ
	InitializeDirectionalLight(dxCommon);
	InitializeLocalLight(dxCommon);
}

//================================================================================
// DirectionalLight (平行光源) の初期化
//================================================================================
void LightManager::InitializeDirectionalLight(DirectXCommon* dxCommon) {
	// 定数バッファ（GPUメモリ）の作成
	directionalLightBuffer_ = dxCommon->CreateBufferResource(sizeof(DirectionalLight));

	// マッピングしてC++から書き込めるようにする
	directionalLightBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));

	// デフォルト設定 (白光、斜め下、輝度0.0f)
	directionalLightData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
	directionalLightData_->direction = {0.0f, -1.0f, 0.5f};
	directionalLightData_->intensity = 0.0f;
}

//================================================================================
// LocalLight (PointLight / SpotLight) の初期化
//================================================================================
void LightManager::InitializeLocalLight(DirectXCommon* dxCommon) {
	// バッファ作成
	localLightBuffer_ = dxCommon->CreateBufferResource(sizeof(LocalLight));
	localLightBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&localLightData_));

	// デフォルト値（最初はPointLightとして設定しておく）
	localLightData_->type = static_cast<uint32_t>(LocalLightType::kPoint);
	localLightData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
	localLightData_->position = {0.0f, 2.0f, 0.0f};
	localLightData_->intensity = 1.0f;
	localLightData_->distance = 10.0f;
	localLightData_->decay = 1.0f;
	// スポット用の初期値も一応入れておく
	localLightData_->direction = {0.0f, -1.0f, 0.0f};
	localLightData_->cosAngle = std::cos(std::numbers::pi_v<float> / 4.0f);
	localLightData_->cosFalloffStart = 1.0f;
}