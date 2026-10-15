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
	IDxcBlob* GetPixelShader() const override { return psBlob_.Get(); }

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob_ = nullptr;
};

//================================================================================
// 2. ラジアルブラー
//================================================================================
class RadialBlurEffect : public IPostProcessEffect
{
public:
	void Initialize(DirectXCommon* dxCommon) override;
	IDxcBlob* GetPixelShader() const override { return psBlob_.Get(); }

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob_ = nullptr;
};

//================================================================================
// 3. ボックスフィルター
//================================================================================
class BoxFilterEffect : public IPostProcessEffect
{
public:
	void Initialize(DirectXCommon* dxCommon) override;
	IDxcBlob* GetPixelShader() const override { return psBlob_.Get(); }

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob_ = nullptr;
};

//================================================================================
// 4. ガウシアンフィルター
//================================================================================
class GaussianFilterEffect : public IPostProcessEffect
{
public:
	void Initialize(DirectXCommon* dxCommon) override;
	IDxcBlob* GetPixelShader() const override { return psBlob_.Get(); }

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob_ = nullptr;
};

//================================================================================
// 5. グレースケール
//================================================================================
class GrayscaleEffect : public IPostProcessEffect
{
public:
	void Initialize(DirectXCommon* dxCommon) override;
	IDxcBlob* GetPixelShader() const override { return psBlob_.Get(); }

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob_ = nullptr;
};

//================================================================================
// 6. アウトライン
//================================================================================
class OutlineEffect : public IPostProcessEffect
{
public:
	void Initialize(DirectXCommon* dxCommon) override;
	IDxcBlob* GetPixelShader() const override { return psBlob_.Get(); }

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob_ = nullptr;
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
	void PreDraw() override;

	IDxcBlob* GetPixelShader() const override { return psBlob_.Get(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const override {
		return constantBuffer_ ? constantBuffer_->GetGPUVirtualAddress() : 0;
	}

	void SetColor(const Vector4& color);
	const Vector4& GetColor() const { return color_; }

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_ = nullptr;
	VignetteParams* paramsData_ = nullptr;

	Vector4 color_ = {0.0f, 0.0f, 0.0f, 1.0f}; // デフォルトは黒
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
	void PreDraw() override;

	IDxcBlob* GetPixelShader() const override { return psBlob_.Get(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const override {
		return constantBuffer_ ? constantBuffer_->GetGPUVirtualAddress() : 0;
	}

	bool HasExtraTexture() const override { return true; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetExtraTextureHandle() const override;

	void SetThreshold(float threshold);
	float GetThreshold() const { return threshold_; }

	void SetNoiseTexture(const std::string& filePath);
	const std::string& GetNoiseTexture() const { return noiseTextureFilePath_; }

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_ = nullptr;
	DissolveParams* paramsData_ = nullptr;

	float threshold_ = 0.0f;
	std::string noiseTextureFilePath_ = "resources/sprites/noise0.png";
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
	void PreDraw() override;

	IDxcBlob* GetPixelShader() const override { return psBlob_.Get(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const override {
		return constantBuffer_ ? constantBuffer_->GetGPUVirtualAddress() : 0;
	}

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_ = nullptr;
	RandomNoiseParams* paramsData_ = nullptr;
	float time_ = 0.0f;
};

//================================================================================
// 10. グリッチエフェクト（サイバーパンク風デジタルノイズ・色収差）
//================================================================================
class GlitchEffect : public IPostProcessEffect
{
public:
	struct GlitchParams {
		float time;
		float glitchIntensity;
		float chromaticAberration;
		float scanlineIntensity;

		float glitchSpeed;
		float glitchFrequency;
		float blockCount;
		float shiftScale;
	};

public:
	void Initialize(DirectXCommon* dxCommon) override;
	void PreDraw() override;

	IDxcBlob* GetPixelShader() const override { return psBlob_.Get(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const override {
		return constantBuffer_ ? constantBuffer_->GetGPUVirtualAddress() : 0;
	}

	void SetIntensity(float intensity) { intensity_ = intensity; }
	float GetIntensity() const { return intensity_; }

	void SetChromaticAberration(float ca) { chromaticAberration_ = ca; }
	float GetChromaticAberration() const { return chromaticAberration_; }

	void SetScanlineIntensity(float scanline) { scanlineIntensity_ = scanline; }
	float GetScanlineIntensity() const { return scanlineIntensity_; }

	// リズム・更新速度 (1秒あたりのコマ数)
	void SetSpeed(float speed) { glitchSpeed_ = speed; }
	float GetSpeed() const { return glitchSpeed_; }

	// 発生頻度 (0.0 ~ 1.0)
	void SetFrequency(float freq) { glitchFrequency_ = freq; }
	float GetFrequency() const { return glitchFrequency_; }

	// 画面縦の分割数
	void SetBlockCount(float count) { blockCount_ = count; }
	float GetBlockCount() const { return blockCount_; }

	// 横ズレ幅の倍率
	void SetShiftScale(float scale) { shiftScale_ = scale; }
	float GetShiftScale() const { return shiftScale_; }

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_ = nullptr;
	GlitchParams* paramsData_ = nullptr;

	float time_ = 0.0f;
	float intensity_ = 0.6f;
	float chromaticAberration_ = 0.008f;
	float scanlineIntensity_ = 0.15f;
	float glitchSpeed_ = 12.0f;
	float glitchFrequency_ = 0.35f;
	float blockCount_ = 35.0f;
	float shiftScale_ = 1.0f;
};

//================================================================================
// 11. CRT・グリッチ画面遷移エフェクト（RGBグリッチ＆ブラウン管シャットダウン/再点灯）
//================================================================================
class CrtTransitionEffect : public IPostProcessEffect
{
public:
	struct CrtTransitionParams {
		float time;
		float glitchIntensity;     // RGBグリッチ強度 (0.0 ~ 1.0)
		float chromaticAberration; // 色収差量 (0.0 ~ 0.1)
		float scanlineIntensity;   // 走査線強度 (0.0 ~ 1.0)

		float collapseX;           // 水平スケール (1.0 = 全画面, 0.0 = 収縮)
		float collapseY;           // 垂直スケール (1.0 = 全画面, 0.0 = 収縮)
		float flashIntensity;      // 白熱・発光強度 (0.0 ~ 2.0)
		float screenCurvature;     // ブラウン管歪み (0.0 ~ 0.1)
	};

public:
	void Initialize(DirectXCommon* dxCommon) override;
	void PreDraw() override;

	IDxcBlob* GetPixelShader() const override { return psBlob_.Get(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const override {
		return constantBuffer_ ? constantBuffer_->GetGPUVirtualAddress() : 0;
	}

	void SetGlitchIntensity(float val) { glitchIntensity_ = val; }
	float GetGlitchIntensity() const { return glitchIntensity_; }

	void SetChromaticAberration(float val) { chromaticAberration_ = val; }
	float GetChromaticAberration() const { return chromaticAberration_; }

	void SetScanlineIntensity(float val) { scanlineIntensity_ = val; }
	float GetScanlineIntensity() const { return scanlineIntensity_; }

	void SetCollapseX(float val) { collapseX_ = val; }
	float GetCollapseX() const { return collapseX_; }

	void SetCollapseY(float val) { collapseY_ = val; }
	float GetCollapseY() const { return collapseY_; }

	void SetFlashIntensity(float val) { flashIntensity_ = val; }
	float GetFlashIntensity() const { return flashIntensity_; }

	void SetScreenCurvature(float val) { screenCurvature_ = val; }
	float GetScreenCurvature() const { return screenCurvature_; }

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_ = nullptr;
	CrtTransitionParams* paramsData_ = nullptr;

	float time_ = 0.0f;
	float glitchIntensity_ = 0.0f;
	float chromaticAberration_ = 0.0f;
	float scanlineIntensity_ = 0.25f;
	float collapseX_ = 1.0f;
	float collapseY_ = 1.0f;
	float flashIntensity_ = 0.0f;
	float screenCurvature_ = 0.04f;
};
