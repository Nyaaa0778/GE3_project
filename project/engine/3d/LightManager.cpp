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
	InitializePointLight(dxCommon);
	InitializeSpotLight(dxCommon);
}

void LightManager::SetSpotLightCosFalloffStart(float cosFalloffStart) {
	spotLightData_->cosFalloffStart = std::max(cosFalloffStart, spotLightData_->cosAngle);
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
// PointLight (点光源) の初期化
//================================================================================
void LightManager::InitializePointLight(DirectXCommon* dxCommon) {
	// 定数バッファ（GPUメモリ）の作成
	pointLightBuffer_ = dxCommon->CreateBufferResource(sizeof(PointLight));

	// マッピングしてC++から書き込めるようにする
	pointLightBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData_));

	// デフォルト設定 (白光、斜め下、輝度1.5f)
	pointLightData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
	pointLightData_->position = {0.0f, 2.0f, 0.0f};
	pointLightData_->intensity = 0.0f;
	pointLightData_->radius = 5.0f;
	pointLightData_->decay = 1.0f;
	pointLightData_->lightType = 1;
}

void LightManager::InitializeSpotLight(DirectXCommon* dxCommon) {
	spotLightBuffer_ = dxCommon->CreateBufferResource(sizeof(SpotLight));
	spotLightBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData_));

	// デフォルト設定
	spotLightData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
	spotLightData_->position = {-2.0f, 1.25f, 0.0f};
	spotLightData_->intensity = 4.0f;
	spotLightData_->direction = Normalize({1.0f, -1.0f, 0.0f}); // 下向き
	spotLightData_->distance = 7.0f;
	spotLightData_->decay = 2.0f;
	spotLightData_->cosAngle = std::cos(std::numbers::pi_v<float> / 3.0f);
	spotLightData_->cosFalloffStart = 1.0f;
	spotLightData_->lightType = 2;
}