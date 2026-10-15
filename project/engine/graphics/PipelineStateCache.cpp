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

	CreateCommonRootSignature();
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
	hashCombine(static_cast<int>(key.topologyType));

	return seed;
}

ID3D12PipelineState* PipelineStateCache::GetOrCreate(const PipelineDescKey& key) {
	// ルートシグネチャが未指定なら共通のものを補完
	PipelineDescKey actualKey = key;
	if (actualKey.rootSignature == nullptr)
	{
		actualKey.rootSignature = commonRootSignature_.Get();
	}

	// キャッシュの中から探す
	auto it = psoCache_.find(actualKey);
	if (it != psoCache_.end())
	{
		return it->second.Get();
	}

	// キャッシュになければ新規生成
	assert(actualKey.rootSignature != nullptr && "RootSignature is required");
	assert(actualKey.vsBlob != nullptr && "VertexShader is required");
	assert(actualKey.psBlob != nullptr && "PixelShader is required");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.pRootSignature = actualKey.rootSignature;

	desc.VS = {actualKey.vsBlob->GetBufferPointer(), actualKey.vsBlob->GetBufferSize()};
	desc.PS = {actualKey.psBlob->GetBufferPointer(), actualKey.psBlob->GetBufferSize()};

	desc.InputLayout.pInputElementDescs = actualKey.inputElements;
	desc.InputLayout.NumElements = actualKey.numInputElements;

	desc.BlendState = BlendModeHelper::MakeBlendDesc(actualKey.blendMode);

	desc.RasterizerState.CullMode = actualKey.cullMode;
	desc.RasterizerState.FillMode = actualKey.fillMode;

	desc.DepthStencilState.DepthEnable = actualKey.depthEnable ? TRUE : FALSE;
	desc.DepthStencilState.DepthWriteMask = actualKey.depthWrite ?
		D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	desc.DepthStencilState.DepthFunc = actualKey.depthFunc;

	desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.PrimitiveTopologyType = actualKey.topologyType;
	desc.SampleDesc.Count = 1;
	desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	ComPtr<ID3D12PipelineState> newPso;
	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&newPso));
	assert(SUCCEEDED(hr) && "Failed to create PipelineState");

	ID3D12PipelineState* result = newPso.Get();
	psoCache_[actualKey] = std::move(newPso);

	return result;
}

void PipelineStateCache::CreateCommonRootSignature() {
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_ROOT_PARAMETER rootParameters[7] = {};

	// [0] Material（PS b0）
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;

	// [1] Transform（VS b0）
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 0;

	// [2] Camera（VS / PS b1）
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[2].Descriptor.ShaderRegister = 1;

	// [3] Light（PS b2）
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.ShaderRegister = 2;

	D3D12_DESCRIPTOR_RANGE ranges[3] = {};
	for (int i = 0; i < 3; ++i)
	{
		// [4] Texture SRV（t0）
		// [5] Environment SRV（t1）
		// [6] Dissolve SRV（t2）
		ranges[i].BaseShaderRegister = i;
		ranges[i].NumDescriptors = 1;
		ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParameters[4 + i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[4 + i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[4 + i].DescriptorTable.pDescriptorRanges = &ranges[i];
		rootParameters[4 + i].DescriptorTable.NumDescriptorRanges = 1;
	}

	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);

	// Static Samplers (s0: Wrap for 3D/Sprite, s1: Clamp for PostProcess)
	D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};

	// s0: Linear Wrap
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0; // s0
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// s1: Linear Clamp
	staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[1].ShaderRegister = 1; // s1
	staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	assert(SUCCEEDED(hr));

	hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&commonRootSignature_));
	assert(SUCCEEDED(hr));

}
