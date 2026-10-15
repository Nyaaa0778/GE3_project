#include "PostProcessEffects.h"
#include "DirectXCommon.h"
#include "TextureManager.h"
#include <cassert>

//================================================================================
// 1. NormalEffect
//================================================================================
void NormalEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	psBlob_ = dxCommon_->CompileShader(L"resources/shaders/finalBlit/FinalBlit.PS.hlsl", L"ps_6_0");
	assert(psBlob_);
}

//================================================================================
// 2. RadialBlurEffect
//================================================================================
void RadialBlurEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	psBlob_ = dxCommon_->CompileShader(L"resources/shaders/finalBlit/RadialBlur.PS.hlsl", L"ps_6_0");
	assert(psBlob_);
}

//================================================================================
// 3. BoxFilterEffect
//================================================================================
void BoxFilterEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	psBlob_ = dxCommon_->CompileShader(L"resources/shaders/finalBlit/BoxFilter.PS.hlsl", L"ps_6_0");
	assert(psBlob_);
}

//================================================================================
// 4. GaussianFilterEffect
//================================================================================
void GaussianFilterEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	psBlob_ = dxCommon_->CompileShader(L"resources/shaders/finalBlit/GaussianFilter.PS.hlsl", L"ps_6_0");
	assert(psBlob_);
}

//================================================================================
// 5. GrayscaleEffect
//================================================================================
void GrayscaleEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	psBlob_ = dxCommon_->CompileShader(L"resources/shaders/finalBlit/Grayscale.PS.hlsl", L"ps_6_0");
	assert(psBlob_);
}

//================================================================================
// 6. OutlineEffect
//================================================================================
void OutlineEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	psBlob_ = dxCommon_->CompileShader(L"resources/shaders/finalBlit/Outline.PS.hlsl", L"ps_6_0");
	assert(psBlob_);
}

//================================================================================
// 7. VignetteEffect (ビネット枠色カスタマイズ対応)
//================================================================================
void VignetteEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;

	// 定数バッファの作成
	constantBuffer_ = dxCommon_->CreateBufferResource(sizeof(VignetteParams));
	constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&paramsData_));
	
	// 初期パラメータ設定 (デフォルトは黒)
	paramsData_->color = color_;

	psBlob_ = dxCommon_->CompileShader(L"resources/shaders/finalBlit/Vignetting.PS.hlsl", L"ps_6_0");
	assert(psBlob_);
}

void VignetteEffect::PreDraw() {
	if (paramsData_) {
		paramsData_->color = color_;
	}
}

void VignetteEffect::SetColor(const Vector4& color) {
	color_ = color;
	if (paramsData_) {
		paramsData_->color = color;
	}
}

//================================================================================
// 8. DissolveEffect
//================================================================================
void DissolveEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;

	// 定数バッファの作成
	constantBuffer_ = dxCommon_->CreateBufferResource(sizeof(DissolveParams));
	constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&paramsData_));
	
	// 初期パラメータ設定
	paramsData_->threshold = threshold_;
	paramsData_->edgeWidth = 0.04f;
	paramsData_->edgeColor = {0.0f, 0.25f, 1.0f, 1.0f}; // 青色の発光

	psBlob_ = dxCommon_->CompileShader(L"resources/shaders/finalBlit/Dissolve.PS.hlsl", L"ps_6_0");
	assert(psBlob_);
}

void DissolveEffect::PreDraw() {
	if (paramsData_) {
		paramsData_->threshold = threshold_;
		paramsData_->edgeWidth = 0.04f;
		paramsData_->edgeColor = {0.0f, 0.25f, 1.0f, 1.0f};
	}
}

void DissolveEffect::SetThreshold(float threshold) {
	threshold_ = threshold;
	if (paramsData_) {
		paramsData_->threshold = threshold;
	}
}

void DissolveEffect::SetNoiseTexture(const std::string& filePath) {
	noiseTextureFilePath_ = filePath;
}

D3D12_GPU_DESCRIPTOR_HANDLE DissolveEffect::GetExtraTextureHandle() const {
	return TextureManager::GetInstance()->GetSrvHandleGPU(noiseTextureFilePath_);
}

//================================================================================
// 9. RandomNoiseEffect
//================================================================================
void RandomNoiseEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;

	// 定数バッファの作成
	constantBuffer_ = dxCommon_->CreateBufferResource(sizeof(RandomNoiseParams));
	constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&paramsData_));
	paramsData_->time = 0.0f;

	psBlob_ = dxCommon_->CompileShader(L"resources/shaders/finalBlit/RandomNoise.PS.hlsl", L"ps_6_0");
	assert(psBlob_);
}

void RandomNoiseEffect::PreDraw() {
	time_ += 0.016f;
	if (paramsData_) {
		paramsData_->time = time_;
	}
}

//================================================================================
// 10. GlitchEffect
//================================================================================
void GlitchEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;

	// 定数バッファの作成
	constantBuffer_ = dxCommon_->CreateBufferResource(sizeof(GlitchParams));
	constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&paramsData_));
	paramsData_->time = 0.0f;
	paramsData_->glitchIntensity = intensity_;
	paramsData_->chromaticAberration = chromaticAberration_;
	paramsData_->scanlineIntensity = scanlineIntensity_;
	paramsData_->glitchSpeed = glitchSpeed_;
	paramsData_->glitchFrequency = glitchFrequency_;
	paramsData_->blockCount = blockCount_;
	paramsData_->shiftScale = shiftScale_;

	psBlob_ = dxCommon_->CompileShader(L"resources/shaders/finalBlit/Glitch.PS.hlsl", L"ps_6_0");
	assert(psBlob_);
}

void GlitchEffect::PreDraw() {
	time_ += 0.016f;
	if (paramsData_) {
		paramsData_->time = time_;
		paramsData_->glitchIntensity = intensity_;
		paramsData_->chromaticAberration = chromaticAberration_;
		paramsData_->scanlineIntensity = scanlineIntensity_;
		paramsData_->glitchSpeed = glitchSpeed_;
		paramsData_->glitchFrequency = glitchFrequency_;
		paramsData_->blockCount = blockCount_;
		paramsData_->shiftScale = shiftScale_;
	}
}

//================================================================================
// 11. CRT・グリッチ画面遷移エフェクト
//================================================================================
void CrtTransitionEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;

	// 定数バッファの生成 (256バイトアライメント)
	uint32_t size = (sizeof(CrtTransitionParams) + 0xff) & ~0xff;
	constantBuffer_ = dxCommon_->CreateBufferResource(size);
	constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&paramsData_));

	paramsData_->time = 0.0f;
	paramsData_->glitchIntensity = glitchIntensity_;
	paramsData_->chromaticAberration = chromaticAberration_;
	paramsData_->scanlineIntensity = scanlineIntensity_;
	paramsData_->collapseX = collapseX_;
	paramsData_->collapseY = collapseY_;
	paramsData_->flashIntensity = flashIntensity_;
	paramsData_->screenCurvature = screenCurvature_;

	psBlob_ = dxCommon_->CompileShader(L"resources/shaders/finalBlit/CrtTransition.PS.hlsl", L"ps_6_0");
	assert(psBlob_);
}

void CrtTransitionEffect::PreDraw() {
	time_ += 0.016f;
	if (paramsData_) {
		paramsData_->time = time_;
		paramsData_->glitchIntensity = glitchIntensity_;
		paramsData_->chromaticAberration = chromaticAberration_;
		paramsData_->scanlineIntensity = scanlineIntensity_;
		paramsData_->collapseX = collapseX_;
		paramsData_->collapseY = collapseY_;
		paramsData_->flashIntensity = flashIntensity_;
		paramsData_->screenCurvature = screenCurvature_;
	}
}
