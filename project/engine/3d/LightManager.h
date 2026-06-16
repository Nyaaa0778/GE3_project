#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

#include "Vector3.h"
#include "Vector4.h"

class DirectXCommon;

#include <cassert>

//================================================================================
// 定数
//================================================================================
static constexpr int kMaxPointLights = 8;
static constexpr int kMaxSpotLights = 4;

//================================================================================
// LightManager クラス
//================================================================================
class LightManager {
public:
	//================================================================================
	// 構造体
	//================================================================================
	struct DirectionalLight {
		Vector4 color;     // ライトの色
		Vector3 direction; // ライトの向き
		float intensity;   // 輝度
	};

	struct PointLight {
		Vector4 color;     // ライトの色 (16 bytes)
		Vector3 position;  // ライトの位置 (12 bytes)
		float intensity;   // 輝度 (4 bytes)
		float distance;    // ライトの届く距離 (4 bytes)
		float decay;       // 減衰率 (4 bytes)
		int32_t enabled;   // 有効フラグ (4 bytes)
		float pad;         // パディング (4 bytes) -> 合計48バイト (16バイト境界アライメント)
	};

	struct SpotLight {
		Vector4 color;         // ライトの色 (16 bytes)
		Vector3 position;      // ライトの位置 (12 bytes)
		float intensity;       // 輝度 (4 bytes)
		Vector3 direction;     // 向き (12 bytes)
		float distance;        // 届く距離 (4 bytes)
		float decay;           // 減衰率 (4 bytes)
		float cosAngle;        // スポットライトの余弦 (4 bytes)
		float cosFalloffStart; // falloffの開始角度 (4 bytes)
		int32_t enabled;       // 有効フラグ (4 bytes)
	};

	struct LightData {
		DirectionalLight directionalLight; // 32 bytes
		PointLight pointLights[kMaxPointLights]; // 8 * 48 = 384 bytes
		SpotLight spotLights[kMaxSpotLights];   // 4 * 64 = 256 bytes
	}; // 合計: 672 bytes (16の倍数)

	//================================================================================
	// シングルトン
	//================================================================================
	// インスタンスの取得
	static LightManager* GetInstance();

	//================================================================================
	// 初期化
	//================================================================================
	// 全ライトの初期化
	void Initialize(DirectXCommon* dxCommon);

	//================================================================================
	// GPUアドレスの取得
	//================================================================================
	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferVideoAddress() const { return lightDataBuffer_->GetGPUVirtualAddress(); }

	//================================================================================
	// DirectionalLight (平行光源) の取得・設定
	//================================================================================
	const Vector4& GetDirectionalLightColor() const { return lightData_->directionalLight.color; }
	const Vector3& GetDirectionalLightDirection() const { return lightData_->directionalLight.direction; }
	float GetDirectionalLightIntensity() const { return lightData_->directionalLight.intensity; }

	void SetDirectionalLightColor(const Vector4& color) { lightData_->directionalLight.color = color; }
	void SetDirectionalLightDirection(const Vector3& direction) { lightData_->directionalLight.direction = direction; }
	void SetDirectionalLightIntensity(float intensity) { lightData_->directionalLight.intensity = intensity; }

	//================================================================================
	// PointLight (点光源) の取得・設定
	//================================================================================
	const Vector4& GetPointLightColor(int index) const {
		assert(index >= 0 && index < kMaxPointLights);
		return lightData_->pointLights[index].color;
	}
	const Vector3& GetPointLightPosition(int index) const {
		assert(index >= 0 && index < kMaxPointLights);
		return lightData_->pointLights[index].position;
	}
	float GetPointLightIntensity(int index) const {
		assert(index >= 0 && index < kMaxPointLights);
		return lightData_->pointLights[index].intensity;
	}
	float GetPointLightDistance(int index) const {
		assert(index >= 0 && index < kMaxPointLights);
		return lightData_->pointLights[index].distance;
	}
	float GetPointLightDecay(int index) const {
		assert(index >= 0 && index < kMaxPointLights);
		return lightData_->pointLights[index].decay;
	}
	bool GetPointLightEnabled(int index) const {
		assert(index >= 0 && index < kMaxPointLights);
		return lightData_->pointLights[index].enabled != 0;
	}

	void SetPointLightColor(int index, const Vector4& color) {
		assert(index >= 0 && index < kMaxPointLights);
		lightData_->pointLights[index].color = color;
	}
	void SetPointLightPosition(int index, const Vector3& position) {
		assert(index >= 0 && index < kMaxPointLights);
		lightData_->pointLights[index].position = position;
	}
	void SetPointLightIntensity(int index, float intensity) {
		assert(index >= 0 && index < kMaxPointLights);
		lightData_->pointLights[index].intensity = intensity;
	}
	void SetPointLightDistance(int index, float distance) {
		assert(index >= 0 && index < kMaxPointLights);
		lightData_->pointLights[index].distance = distance;
	}
	void SetPointLightDecay(int index, float decay) {
		assert(index >= 0 && index < kMaxPointLights);
		lightData_->pointLights[index].decay = decay;
	}
	void SetPointLightEnabled(int index, bool enabled) {
		assert(index >= 0 && index < kMaxPointLights);
		lightData_->pointLights[index].enabled = enabled ? 1 : 0;
	}

	//================================================================================
	// SpotLight (スポットライト) の取得・設定
	//================================================================================
	const Vector4& GetSpotLightColor(int index) const {
		assert(index >= 0 && index < kMaxSpotLights);
		return lightData_->spotLights[index].color;
	}
	const Vector3& GetSpotLightPosition(int index) const {
		assert(index >= 0 && index < kMaxSpotLights);
		return lightData_->spotLights[index].position;
	}
	float GetSpotLightIntensity(int index) const {
		assert(index >= 0 && index < kMaxSpotLights);
		return lightData_->spotLights[index].intensity;
	}
	const Vector3& GetSpotLightDirection(int index) const {
		assert(index >= 0 && index < kMaxSpotLights);
		return lightData_->spotLights[index].direction;
	}
	float GetSpotLightDistance(int index) const {
		assert(index >= 0 && index < kMaxSpotLights);
		return lightData_->spotLights[index].distance;
	}
	float GetSpotLightDecay(int index) const {
		assert(index >= 0 && index < kMaxSpotLights);
		return lightData_->spotLights[index].decay;
	}
	float GetSpotLightCosAngle(int index) const {
		assert(index >= 0 && index < kMaxSpotLights);
		return lightData_->spotLights[index].cosAngle;
	}
	float GetSpotLightCosFalloffStart(int index) const {
		assert(index >= 0 && index < kMaxSpotLights);
		return lightData_->spotLights[index].cosFalloffStart;
	}
	bool GetSpotLightEnabled(int index) const {
		assert(index >= 0 && index < kMaxSpotLights);
		return lightData_->spotLights[index].enabled != 0;
	}

	void SetSpotLightColor(int index, const Vector4& color) {
		assert(index >= 0 && index < kMaxSpotLights);
		lightData_->spotLights[index].color = color;
	}
	void SetSpotLightPosition(int index, const Vector3& position) {
		assert(index >= 0 && index < kMaxSpotLights);
		lightData_->spotLights[index].position = position;
	}
	void SetSpotLightIntensity(int index, float intensity) {
		assert(index >= 0 && index < kMaxSpotLights);
		lightData_->spotLights[index].intensity = intensity;
	}
	void SetSpotLightDirection(int index, const Vector3& direction) {
		assert(index >= 0 && index < kMaxSpotLights);
		lightData_->spotLights[index].direction = direction;
	}
	void SetSpotLightDistance(int index, float distance) {
		assert(index >= 0 && index < kMaxSpotLights);
		lightData_->spotLights[index].distance = distance;
	}
	void SetSpotLightDecay(int index, float decay) {
		assert(index >= 0 && index < kMaxSpotLights);
		lightData_->spotLights[index].decay = decay;
	}
	void SetSpotLightCosAngle(int index, float cosAngle) {
		assert(index >= 0 && index < kMaxSpotLights);
		lightData_->spotLights[index].cosAngle = cosAngle;
	}
	void SetSpotLightCosFalloffStart(int index, float cosFalloffStart) {
		assert(index >= 0 && index < kMaxSpotLights);
		lightData_->spotLights[index].cosFalloffStart = cosFalloffStart;
	}
	void SetSpotLightEnabled(int index, bool enabled) {
		assert(index >= 0 && index < kMaxSpotLights);
		lightData_->spotLights[index].enabled = enabled ? 1 : 0;
	}

private:
	// シングルトンのためコンストラクタを隠蔽
	LightManager() = default;
	~LightManager() = default;
	// コピーと代入を禁止
	LightManager(const LightManager&) = delete;
	LightManager& operator=(const LightManager&) = delete;

private:
	//================================================================================
	// メンバ変数
	//================================================================================
	Microsoft::WRL::ComPtr<ID3D12Resource> lightDataBuffer_; // 定数バッファ
	LightData* lightData_ = nullptr;                        // マップ用ポインタ
};