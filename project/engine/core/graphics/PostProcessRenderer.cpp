#include "PostProcessRenderer.h"
#include "PostProcessEffects.h"
#include "DirectXCommon.h"
#include "ShaderResourceViewManager.h"
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

	// アクティブモードの初期値
	activeModes_ = { PostProcessMode::kNormal };

	// 各種エフェクトインスタンスの作成
	effects_[PostProcessMode::kNormal] = std::make_unique<NormalEffect>();
	effects_[PostProcessMode::kRadialBlur] = std::make_unique<RadialBlurEffect>();
	effects_[PostProcessMode::kBoxFilter] = std::make_unique<BoxFilterEffect>();
	effects_[PostProcessMode::kGaussianFilter] = std::make_unique<GaussianFilterEffect>();
	effects_[PostProcessMode::kGrayscale] = std::make_unique<GrayscaleEffect>();
	effects_[PostProcessMode::kOutline] = std::make_unique<OutlineEffect>();
	effects_[PostProcessMode::kVignetting] = std::make_unique<VignetteEffect>();
	effects_[PostProcessMode::kDissolve] = std::make_unique<DissolveEffect>();
	effects_[PostProcessMode::kRandomNoise] = std::make_unique<RandomNoiseEffect>();

	// 一括初期化
	for (auto& pair : effects_) {
		pair.second->Initialize(dxCommon_);
	}

	// -----------------------------------------------------------
	// マルチパス（チェイニング）描画用の中間バッファの作成
	// -----------------------------------------------------------
	// 1. RTV用のデスクリプタヒープの作成 (記述子数2)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 2;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = dxCommon_->GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_));
	assert(SUCCEEDED(hr));

	// 2. 中間テクスチャリソース・RTV・SRVの作成
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Width = dxCommon_->GetClientWidth();
	resourceDesc.Height = dxCommon_->GetClientHeight();
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	const float kRenderTargetClearValue[4] = { 0.1f, 0.25f, 0.5f, 1.0f };
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	clearValue.Color[0] = kRenderTargetClearValue[0];
	clearValue.Color[1] = kRenderTargetClearValue[1];
	clearValue.Color[2] = kRenderTargetClearValue[2];
	clearValue.Color[3] = kRenderTargetClearValue[3];

	UINT rtvDescriptorSize = dxCommon_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	for (int i = 0; i < 2; ++i) {
		hr = dxCommon_->GetDevice()->CreateCommittedResource(
			&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue,
			IID_PPV_ARGS(&intermediateResources_[i])
		);
		assert(SUCCEEDED(hr));

		// RTVの作成
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += rtvDescriptorSize * i;
		intermediateRtvHandles_[i] = rtvHandle;

		dxCommon_->GetDevice()->CreateRenderTargetView(intermediateResources_[i].Get(), &rtvDesc, rtvHandle);

		// SRVの作成 (ShaderResourceViewManagerからアロケート)
		intermediateSrvIndices_[i] = ShaderResourceViewManager::GetInstance()->Allocate();
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU = ShaderResourceViewManager::GetInstance()->GetCPUDescriptorHandle(intermediateSrvIndices_[i]);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		dxCommon_->GetDevice()->CreateShaderResourceView(intermediateResources_[i].Get(), &srvDesc, srvHandleCPU);
		intermediateSrvHandlesGPU_[i] = ShaderResourceViewManager::GetInstance()->GetGPUDescriptorHandle(intermediateSrvIndices_[i]);
	}
}

void PostProcessRenderer::Draw(D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) {
	// 適用するエフェクトのリストを整理
	std::vector<PostProcessMode> modesToRun;
	for (auto mode : activeModes_) {
		// kNormal 以外（またはそれしかない場合）を追加
		if (mode != PostProcessMode::kNormal) {
			modesToRun.push_back(mode);
		}
	}

	// 登録がない場合は Normal (単なるコピー描画)
	if (modesToRun.empty()) {
		modesToRun.push_back(PostProcessMode::kNormal);
	}

	size_t numPasses = modesToRun.size();

	if (numPasses == 1) {
		// 単一パスの場合は、従来通り直接バックバッファに描画
		auto it = effects_.find(modesToRun[0]);
		if (it != effects_.end()) {
			it->second->Draw(dxCommon_->GetCommandList(), srvHandle);
		}
	} else {
		// 複数エフェクト（マルチパス）のチェイニング描画
		auto* cmdList = dxCommon_->GetCommandList();
		D3D12_GPU_DESCRIPTOR_HANDLE currentInput = srvHandle;
		int currentOutputIdx = 0;

		for (size_t i = 0; i < numPasses; ++i) {
			bool isLastPass = (i == numPasses - 1);

			if (!isLastPass) {
				// 中間バッファに描画するため、リソースバリアを RENDER_TARGET に遷移
				D3D12_RESOURCE_BARRIER barrier{};
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrier.Transition.pResource = intermediateResources_[currentOutputIdx].Get();
				barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
				cmdList->ResourceBarrier(1, &barrier);

				// レンダーターゲットを中間バッファに設定
				cmdList->OMSetRenderTargets(1, &intermediateRtvHandles_[currentOutputIdx], false, nullptr);

				// エフェクト描画
				auto it = effects_.find(modesToRun[i]);
				if (it != effects_.end()) {
					it->second->Draw(cmdList, currentInput);
				}

				// 次のパスでサンプリングできるようにリソースバリアを PIXEL_SHADER_RESOURCE に戻す
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				cmdList->ResourceBarrier(1, &barrier);

				// 次のパスへのインプットを今の中間にし、出力をもう片方に切り替え (ピンポン)
				currentInput = intermediateSrvHandlesGPU_[currentOutputIdx];
				currentOutputIdx = 1 - currentOutputIdx;
			} else {
				// 最後のパスはバックバッファに描画
				D3D12_CPU_DESCRIPTOR_HANDLE backBufferRtv = dxCommon_->GetCurrentBackBufferRtv();
				cmdList->OMSetRenderTargets(1, &backBufferRtv, false, nullptr);

				auto it = effects_.find(modesToRun[i]);
				if (it != effects_.end()) {
					it->second->Draw(cmdList, currentInput);
				}
			}
		}
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