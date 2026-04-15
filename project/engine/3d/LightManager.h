#pragma once

#include <d3d12.h>
#include <wrl.h>
#include "Vector3.h"
#include "Vector4.h"

class DirectXCommon;

class LightManager {
public:
	struct DirectionalLight {
		Vector4 color;     // ライトの色
		Vector3 direction; // ライトの向き
		float intensity;   // 輝度
	};

	struct PointLight {
		Vector4 color;    // ライトの色
		Vector3 position; // ライトの位置
		float intensity;  // 輝度
	};

	static LightManager* GetInstance();
	void Initialize(DirectXCommon* dxCommon);
	void TransferData(); // 定数バッファへの転送

	// GPUアドレス取得用
	D3D12_GPU_VIRTUAL_ADDRESS GetPointLightConstantBufferVideoAddress() const {
		return pointLightBuffer_->GetGPUVirtualAddress();
	}

	const Vector4& GetPointLightColor() const { return pointLightData_->color; }
	const Vector3& GetPointLightPosition() const { return pointLightData_->position; }
	float GetPointLightIntensity() const { return pointLightData_->intensity; }

	// 各種Setter
	void SetPointLightColor(const Vector4& color) { pointLightData_->color = color; }
	void SetPointLightPosition(const Vector3& position) { pointLightData_->position = position; }
	void SetPointLightIntensity(float intensity) { pointLightData_->intensity = intensity; }

	// GPUアドレス取得用
	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferVideoAddress() const {
		return directionalLightBuffer_->GetGPUVirtualAddress();
	}

	const Vector4& GetColor() const { return directionalLightData_->color; }
	const Vector3& GetDirection() const { return directionalLightData_->direction; }
	float GetIntensity() const { return directionalLightData_->intensity; }

	// 各種Setter
	void SetColor(const Vector4& color) { directionalLightData_->color = color; }
	void SetDirection(const Vector3& direction) { directionalLightData_->direction = direction; }
	void SetIntensity(float intensity) { directionalLightData_->intensity = intensity; }

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightBuffer_;
	DirectionalLight* directionalLightData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> pointLightBuffer_;
	PointLight* pointLightData_ = nullptr;
};