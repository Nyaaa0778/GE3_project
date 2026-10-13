#include "PipelineStateCache.h"

#include <cassert>

#include "DirectXCommon.h"

//================================================================================
// シングルトン
//================================================================================

std::unique_ptr<PipelineStateCache> PipelineStateCache::instance = nullptr;

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>PipelineStateCacheの唯一のインスタンス</returns>
PipelineStateCache* PipelineStateCache::GetInstance() {
	if (instance == nullptr)
	{
		instance = std::make_unique<PipelineStateCache>();
	}

	return instance.get();
}

/// <summary>
/// 終了
/// </summary>
void PipelineStateCache::Finalize() { instance.reset(); }

void PipelineStateCache::Initialize(DirectXCommon* dxCommon) {
	assert(dxCommon);
	dxCommon_ = dxCommon;
}

// キーの比較
bool PipelineDescKey::operator==(const PipelineDescKey& other) const {
	return rootSignature == other.rootSignature &&
		vsBlob == other.vsBlob &&
		psBlob == other.psBlob &&
		inputElements == other.inputElements &&
		numInputElements == other.numInputElements &&
		blendMode == other.blendMode &&
		cullMode == other.cullMode &&
		fillMode == other.fillMode &&
		depthEnable == other.depthEnable &&
		depthWrite == other.depthWrite &&
		depthFunc == other.depthFunc &&
		topologyType == other.topologyType;
}

// ハッシュ生成
size_t PipelineDescKeyHash::operator()(const PipelineDescKey& key) const {
	size_t seed = 0;
	auto hashCombine = [&seed](auto val) {
		seed ^= std::hash<decltype(val)>{}(val) +0x9e3779b9 + (seed << 6) + (seed >> 2);
		};

	hashCombine(key.rootSignature);
	hashCombine(key.vsBlob);
	hashCombine(key.psBlob);
	hashCombine(key.inputElements);
	hashCombine(static_cast<int>(key.blendMode));
	hashCombine(static_cast<int>(key.cullMode));
	hashCombine(static_cast<int>(key.fillMode));
	hashCombine(key.depthEnable);
	hashCombine(key.depthWrite);
	hashCombine(static_cast<int>(key.depthFunc));

	return seed;
}

ID3D12PipelineState* PipelineStateCache::GetOrCreate(const PipelineDescKey& key) {
	// キャッシュの中から探す
	auto it = psoCache_.find(key);
	if (it != psoCache_.end())
	{
		return it->second.Get();
	}

	// キャッシュになければ新規生成
	assert(key.rootSignature != nullptr && "RootSignature is required");
	assert(key.vsBlob != nullptr && "VertexShader is required");
	assert(key.psBlob != nullptr && "PixelShader is required");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.pRootSignature = key.rootSignature;

	desc.VS = {key.vsBlob->GetBufferPointer(), key.vsBlob->GetBufferSize()};
	desc.PS = {key.psBlob->GetBufferPointer(), key.psBlob->GetBufferSize()};

	desc.BlendState = BlendModeHelper::MakeBlendDesc(key.blendMode);

	desc.RasterizerState.CullMode = key.cullMode;
	desc.RasterizerState.FillMode = key.fillMode;

	desc.DepthStencilState.DepthEnable = key.depthEnable ? TRUE : FALSE;
	desc.DepthStencilState.DepthWriteMask = key.depthWrite ?
		D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	desc.DepthStencilState.DepthFunc = key.depthFunc;

	desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.PrimitiveTopologyType = key.topologyType;
	desc.SampleDesc.Count = 1;
	desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	ComPtr<ID3D12PipelineState> newPso;
	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&newPso));
	assert(SUCCEEDED(hr) && "Failed to create PipelineState");

	ID3D12PipelineState* result = newPso.Get();
	psoCache_[key] = std::move(newPso);

	return result;
}