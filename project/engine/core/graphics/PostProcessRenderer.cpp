#include "PostProcessRenderer.h"
#include "PostProcessEffects.h"
#include "DirectXCommon.h"
#include <cassert>

std::unique_ptr<PostProcessRenderer> PostProcessRenderer::instance = nullptr;

PostProcessRenderer* PostProcessRenderer::GetInstance() {
	if (instance == nullptr) {
		instance = std::make_unique<PostProcessRenderer>();
	}
	return instance.get();
}

void PostProcessRenderer::Finalize() {
	instance.reset();
}

void PostProcessRenderer::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;

	// 各種エフェクトインスタンスの作成
	effects_[PostProcessMode::kNormal] = std::make_unique<NormalEffect>();
	effects_[PostProcessMode::kRadialBlur] = std::make_unique<RadialBlurEffect>();
	effects_[PostProcessMode::kBoxFilter] = std::make_unique<BoxFilterEffect>();
	effects_[PostProcessMode::kGaussianFilter] = std::make_unique<GaussianFilterEffect>();
	effects_[PostProcessMode::kGrayscale] = std::make_unique<GrayscaleEffect>();
	effects_[PostProcessMode::kOutline] = std::make_unique<OutlineEffect>();
	effects_[PostProcessMode::kVignetting] = std::make_unique<VignetteEffect>();
	effects_[PostProcessMode::kDissolve] = std::make_unique<DissolveEffect>();

	// 一括初期化
	for (auto& pair : effects_) {
		pair.second->Initialize(dxCommon_);
	}
}

void PostProcessRenderer::Draw(D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) {
	auto it = effects_.find(mode_);
	if (it != effects_.end()) {
		it->second->Draw(dxCommon_->GetCommandList(), srvHandle);
	}
}

void PostProcessRenderer::SetDissolveThreshold(float threshold) {
	if (auto dissolve = GetEffect<DissolveEffect>(PostProcessMode::kDissolve)) {
		dissolve->SetThreshold(threshold);
	}
}

float PostProcessRenderer::GetDissolveThreshold() const {
	auto self = const_cast<PostProcessRenderer*>(this);
	if (auto dissolve = self->GetEffect<DissolveEffect>(PostProcessMode::kDissolve)) {
		return dissolve->GetThreshold();
	}
	return 0.0f;
}

void PostProcessRenderer::SetDissolveNoiseTexture(const std::string& filePath) {
	if (auto dissolve = GetEffect<DissolveEffect>(PostProcessMode::kDissolve)) {
		dissolve->SetNoiseTexture(filePath);
	}
}

const std::string& PostProcessRenderer::GetDissolveNoiseTexture() const {
	auto self = const_cast<PostProcessRenderer*>(this);
	if (auto dissolve = self->GetEffect<DissolveEffect>(PostProcessMode::kDissolve)) {
		return dissolve->GetNoiseTexture();
	}
	static const std::string empty = "";
	return empty;
}

void PostProcessRenderer::SetVignetteColor(const Vector4& color) {
	if (auto vignette = GetEffect<VignetteEffect>(PostProcessMode::kVignetting)) {
		vignette->SetColor(color);
	}
}

Vector4 PostProcessRenderer::GetVignetteColor() const {
	auto self = const_cast<PostProcessRenderer*>(this);
	if (auto vignette = self->GetEffect<VignetteEffect>(PostProcessMode::kVignetting)) {
		return vignette->GetColor();
	}
	return {0.0f, 0.0f, 0.0f, 1.0f};
}