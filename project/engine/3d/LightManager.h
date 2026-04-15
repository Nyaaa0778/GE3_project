#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

#include "Vector3.h"
#include "Vector4.h"

class DirectXCommon;

enum class LocalLightType {
	kPoint = 0, // 点光源
	kSpot = 1,  // スポットライト
};

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

	struct LocalLight {
		Vector4 color;         // ライトの色
		Vector3 position;      // ライトの位置
		float intensity;       // 輝度
		Vector3 direction;     // 向き (スポットライト用)
		float distance;        // ライトの届く距離
		float decay;           // 減衰率
		float cosAngle;        // スポットライトの余弦
		float cosFalloffStart; // falloffの開始角度
		uint32_t type;          // ローカルライトの種類 (0:Point, 1:Spot)
	};

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
	// DirectionalLight (平行光源) の取得・設定
	//================================================================================
	// GPUアドレスの取得
	D3D12_GPU_VIRTUAL_ADDRESS GetDirectionalLightConstantBufferVideoAddress() const { return directionalLightBuffer_->GetGPUVirtualAddress(); }

	// 各種Getter
	const Vector4& GetDirectionalLightColor() const { return directionalLightData_->color; }
	const Vector3& GetDirectionalLightDirection() const { return directionalLightData_->direction; }
	float GetDirectionalLightIntensity() const { return directionalLightData_->intensity; }

	// 各種Setter
	void SetDirectionalLightColor(const Vector4& color) { directionalLightData_->color = color; }
	void SetDirectionalLightDirection(const Vector3& direction) { directionalLightData_->direction = direction; }
	void SetDirectionalLightIntensity(float intensity) { directionalLightData_->intensity = intensity; }

	//================================================================================
	// LocalLight (PoinLight / SpotLight) の取得・設定
	//================================================================================

	D3D12_GPU_VIRTUAL_ADDRESS GetLocalLightConstantBufferVideoAddress() const { return localLightBuffer_->GetGPUVirtualAddress(); }

	// 各種Getter
	const Vector4& GetLocalLightColor() const { return localLightData_->color; }
	const Vector3& GetLocalLightPosition() const { return localLightData_->position; }
	float GetLocalLightIntensity() const { return localLightData_->intensity; }
	const Vector3& GetLocalLightDirection() const { return localLightData_->direction; }
	float GetLocalLightDistance() const { return localLightData_->distance; }
	float GetLocalLightDecay() const { return localLightData_->decay; }
	float GetLocalLightCosAngle() const { return localLightData_->cosAngle; }
	float GetLocalLightCosFalloffStart() const { return localLightData_->cosFalloffStart; }
	LocalLightType GetLocalLightType() const { return static_cast<LocalLightType>(localLightData_->type); }

	// 各種Setter
	void SetLocalLightColor(const Vector4& color) { localLightData_->color = color; }
	void SetLocalLightPosition(const Vector3& position) { localLightData_->position = position; }
	void SetLocalLightIntensity(float intensity) { localLightData_->intensity = intensity; }
	void SetLocalLightDirection(const Vector3& direction) { localLightData_->direction = direction; }
	void SetLocalLightDistance(float distance) { localLightData_->distance = distance; }
	void SetLocalLightDecay(float decay) { localLightData_->decay = decay; }
	void SetLocalLightCosAngle(float cosAngle) { localLightData_->cosAngle = cosAngle; }
	void SetLocalLightCosFalloffStart(float cosFalloffStart) { localLightData_->cosFalloffStart = cosFalloffStart; }
	void SetLocalLightType(LocalLightType type) { localLightData_->type = static_cast<uint32_t>(type); }

private:
	//================================================================================
	// プライベート関数 (シングルトン・肥大化防止用)
	//================================================================================
	// シングルトンのためコンストラクタを隠蔽
	LightManager() = default;
	~LightManager() = default;
	// コピーと代入を禁止
	LightManager(const LightManager&) = delete;
	LightManager& operator=(const LightManager&) = delete;

	// 初期化処理の分割
	void InitializeDirectionalLight(DirectXCommon* dxCommon);
	void InitializeLocalLight(DirectXCommon* dxCommon);

private:
	//================================================================================
	// メンバ変数
	//================================================================================
	// DirectionalLight用
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightBuffer_; // 定数バッファ
	DirectionalLight* directionalLightData_ = nullptr;              // マップ用ポインタ

	Microsoft::WRL::ComPtr<ID3D12Resource> localLightBuffer_;
	LocalLight* localLightData_ = nullptr;
};