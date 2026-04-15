#pragma once

#include <d3d12.h>
#include <wrl.h>
#include "Vector3.h"
#include "Vector4.h"

class DirectXCommon;

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
		Vector4 color;    // ライトの色
		Vector3 position; // ライトの位置
		float intensity;  // 輝度
		float radius;     // ライトが届く最大距離
		float decay;      // 減衰率
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
	// PointLight (点光源) の取得・設定
	//================================================================================
	// GPUアドレスの取得
	D3D12_GPU_VIRTUAL_ADDRESS GetPointLightConstantBufferVideoAddress() const { return pointLightBuffer_->GetGPUVirtualAddress(); }

	// 各種Getter
	const Vector4& GetPointLightColor() const { return pointLightData_->color; }
	const Vector3& GetPointLightPosition() const { return pointLightData_->position; }
	float GetPointLightIntensity() const { return pointLightData_->intensity; }
	float GetPointLightRadius()const { return pointLightData_->radius; }
	float GetPointLightDecay()const { return pointLightData_->decay; }

	// 各種Setter
	void SetPointLightColor(const Vector4& color) { pointLightData_->color = color; }
	void SetPointLightPosition(const Vector3& position) { pointLightData_->position = position; }
	void SetPointLightIntensity(float intensity) { pointLightData_->intensity = intensity; }
	void SetPointLightRadius(float radius) { pointLightData_->radius = radius; }
	void SetPointLightDecay(float decay) { pointLightData_->decay = decay; }

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
	void InitializePointLight(DirectXCommon* dxCommon);

	//================================================================================
	// メンバ変数
	//================================================================================
	// DirectionalLight用
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightBuffer_; // 定数バッファ
	DirectionalLight* directionalLightData_ = nullptr;              // マップ用ポインタ

	// PointLight用
	Microsoft::WRL::ComPtr<ID3D12Resource> pointLightBuffer_;       // 定数バッファ
	PointLight* pointLightData_ = nullptr;                          // マップ用ポインタ
};