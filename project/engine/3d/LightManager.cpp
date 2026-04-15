#include "LightManager.h"
#include "DirectXCommon.h"

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
	pointLightData_->intensity = 1.5f;
	pointLightData_->radius = 5.0f;
	pointLightData_->decay = 1.0f;
}