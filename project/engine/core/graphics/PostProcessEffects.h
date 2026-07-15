#pragma once

#include "IPostProcessEffect.h"
#include <wrl.h>
#include <string>
#include <Vector4.h>

class PostProcessRenderer;

//================================================================================
// 1. 通常描画
//================================================================================
class NormalEffect : public IPostProcessEffect
{
public:
	void Initialize(DirectXCommon* dxCommon) override;
	void Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) override;

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_ = nullptr;

	void CreateRootSignature();
	void CreatePipelineState();
};

//================================================================================
// 2. ラジアルブラー
//================================================================================
class RadialBlurEffect : public IPostProcessEffect
{
public:
	void Initialize(DirectXCommon* dxCommon) override;
	void Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) override;

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_ = nullptr;

	void CreateRootSignature();
	void CreatePipelineState();
};

//================================================================================
// 3. ボックスフィルター
//================================================================================
class BoxFilterEffect : public IPostProcessEffect
{
public:
	void Initialize(DirectXCommon* dxCommon) override;
	void Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) override;

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_ = nullptr;

	void CreateRootSignature();
	void CreatePipelineState();
};

//================================================================================
// 4. ガウシアンフィルター
//================================================================================
class GaussianFilterEffect : public IPostProcessEffect
{
public:
	void Initialize(DirectXCommon* dxCommon) override;
	void Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) override;

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_ = nullptr;

	void CreateRootSignature();
	void CreatePipelineState();
};

//================================================================================
// 5. グレースケール
//================================================================================
class GrayscaleEffect : public IPostProcessEffect
{
public:
	struct GrayscaleParams {
		float factor;
		float padding[3];
	};

public:
	void Initialize(DirectXCommon* dxCommon) override;
	void Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) override;

	void SetFactor(float factor);
	float GetFactor() const { return factor_; }

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_ = nullptr;
	GrayscaleParams* paramsData_ = nullptr;

	float factor_ = 0.0f;

	void CreateRootSignature();
	void CreatePipelineState();
};

//================================================================================
// 6. アウトライン
//================================================================================
class OutlineEffect : public IPostProcessEffect
{
public:
	void Initialize(DirectXCommon* dxCommon) override;
	void Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) override;

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_ = nullptr;

	void CreateRootSignature();
	void CreatePipelineState();
};

//================================================================================
// 7. ビネット（外枠色カスタマイズ機能付き）
//================================================================================
class VignetteEffect : public IPostProcessEffect
{
public:
	struct VignetteParams {
		Vector4 color;
	};

public:
	void Initialize(DirectXCommon* dxCommon) override;
	void Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) override;

	void SetColor(const Vector4& color);
	const Vector4& GetColor() const { return color_; }

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_ = nullptr;
	VignetteParams* paramsData_ = nullptr;

	Vector4 color_ = {0.0f, 0.0f, 0.0f, 1.0f}; // デフォルトは黒

	void CreateRootSignature();
	void CreatePipelineState();
};

//================================================================================
// 8. ディゾルブ
//================================================================================
class DissolveEffect : public IPostProcessEffect
{
public:
	struct DissolveParams {
		float threshold;
		float edgeWidth;
		float padding[2];
		Vector4 edgeColor;
	};

public:
	void Initialize(DirectXCommon* dxCommon) override;
	void Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) override;

	void SetThreshold(float threshold);
	float GetThreshold() const { return threshold_; }

	void SetNoiseTexture(const std::string& filePath);
	const std::string& GetNoiseTexture() const { return noiseTextureFilePath_; }

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_ = nullptr;
	DissolveParams* paramsData_ = nullptr;

	float threshold_ = 0.0f;
	std::string noiseTextureFilePath_ = "resources/sprites/noise0.png";

	void CreateRootSignature();
	void CreatePipelineState();
};

//================================================================================
// 9. ランダムノイズ
//================================================================================
class RandomNoiseEffect : public IPostProcessEffect
{
public:
	struct RandomNoiseParams {
		float time;
	};

public:
	void Initialize(DirectXCommon* dxCommon) override;
	void Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) override;

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_ = nullptr;
	RandomNoiseParams* paramsData_ = nullptr;
	float time_ = 0.0f;

	void CreateRootSignature();
	void CreatePipelineState();
};
