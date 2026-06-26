#include "PostProcessEffects.h"
#include "DirectXCommon.h"
#include "TextureManager.h"
#include <cassert>

//================================================================================
// 共通ヘルパー関数など
//================================================================================
namespace {
	// 共通ルートシグネチャ（テクスチャ t0 のみ）を作成するヘルパー
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateCommonRootSignature(DirectXCommon* dxCommon) {
		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature {};
		descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
		descriptorRange[0].BaseShaderRegister = 0; // t0
		descriptorRange[0].NumDescriptors = 1;
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER rootParameters[1] = {};
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;
		rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

		descriptionRootSignature.pParameters = rootParameters;
		descriptionRootSignature.NumParameters = _countof(rootParameters);

		D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
		staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
		staticSamplers[0].ShaderRegister = 0; // s0
		staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		descriptionRootSignature.pStaticSamplers = staticSamplers;
		descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		assert(SUCCEEDED(hr));

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
		hr = dxCommon->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
		assert(SUCCEEDED(hr));

		return rootSignature;
	}

	// 共通のPSO設定を埋めるヘルパー
	D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateDefaultPipelineDesc(
		ID3D12RootSignature* rootSig,
		IDxcBlob* vsBlob,
		IDxcBlob* psBlob)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC gdesc {};
		gdesc.pRootSignature = rootSig;
		gdesc.InputLayout = { nullptr, 0 };
		gdesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
		gdesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

		// ブレンド設定 (無効化)
		gdesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		gdesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
		gdesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		gdesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		gdesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		gdesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		gdesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		gdesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

		// ラスタライザ設定
		gdesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		gdesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

		// 深度ステンシル設定 (無効化)
		gdesc.DepthStencilState.DepthEnable = false;
		gdesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		gdesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		gdesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		gdesc.NumRenderTargets = 1;
		gdesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		gdesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		gdesc.SampleDesc.Count = 1;
		gdesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		return gdesc;
	}
}

//================================================================================
// 1. NormalEffect
//================================================================================
void NormalEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	CreateRootSignature();
	CreatePipelineState();
}
void NormalEffect::CreateRootSignature() {
	rootSignature_ = CreateCommonRootSignature(dxCommon_);
}
void NormalEffect::CreatePipelineState() {
	auto vsBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/FinalBlit.VS.hlsl", L"vs_6_0");
	auto psBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/FinalBlit.PS.hlsl", L"ps_6_0");
	assert(vsBlob && psBlob);

	auto gdesc = CreateDefaultPipelineDesc(rootSignature_.Get(), vsBlob.Get(), psBlob.Get());
	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&gdesc, IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr));
}
void NormalEffect::Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) {
	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	cmdList->SetPipelineState(pipelineState_.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);
	cmdList->DrawInstanced(3, 1, 0, 0);
}

//================================================================================
// 2. RadialBlurEffect
//================================================================================
void RadialBlurEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	CreateRootSignature();
	CreatePipelineState();
}
void RadialBlurEffect::CreateRootSignature() {
	rootSignature_ = CreateCommonRootSignature(dxCommon_);
}
void RadialBlurEffect::CreatePipelineState() {
	auto vsBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/FinalBlit.VS.hlsl", L"vs_6_0");
	auto psBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/RadialBlur.PS.hlsl", L"ps_6_0");
	assert(vsBlob && psBlob);

	auto gdesc = CreateDefaultPipelineDesc(rootSignature_.Get(), vsBlob.Get(), psBlob.Get());
	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&gdesc, IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr));
}
void RadialBlurEffect::Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) {
	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	cmdList->SetPipelineState(pipelineState_.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);
	cmdList->DrawInstanced(3, 1, 0, 0);
}

//================================================================================
// 3. BoxFilterEffect
//================================================================================
void BoxFilterEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	CreateRootSignature();
	CreatePipelineState();
}
void BoxFilterEffect::CreateRootSignature() {
	rootSignature_ = CreateCommonRootSignature(dxCommon_);
}
void BoxFilterEffect::CreatePipelineState() {
	auto vsBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/FinalBlit.VS.hlsl", L"vs_6_0");
	auto psBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/BoxFilter.PS.hlsl", L"ps_6_0");
	assert(vsBlob && psBlob);

	auto gdesc = CreateDefaultPipelineDesc(rootSignature_.Get(), vsBlob.Get(), psBlob.Get());
	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&gdesc, IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr));
}
void BoxFilterEffect::Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) {
	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	cmdList->SetPipelineState(pipelineState_.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);
	cmdList->DrawInstanced(3, 1, 0, 0);
}

//================================================================================
// 4. GaussianFilterEffect
//================================================================================
void GaussianFilterEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	CreateRootSignature();
	CreatePipelineState();
}
void GaussianFilterEffect::CreateRootSignature() {
	rootSignature_ = CreateCommonRootSignature(dxCommon_);
}
void GaussianFilterEffect::CreatePipelineState() {
	auto vsBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/FinalBlit.VS.hlsl", L"vs_6_0");
	auto psBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/GaussianFilter.PS.hlsl", L"ps_6_0");
	assert(vsBlob && psBlob);

	auto gdesc = CreateDefaultPipelineDesc(rootSignature_.Get(), vsBlob.Get(), psBlob.Get());
	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&gdesc, IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr));
}
void GaussianFilterEffect::Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) {
	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	cmdList->SetPipelineState(pipelineState_.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);
	cmdList->DrawInstanced(3, 1, 0, 0);
}

//================================================================================
// 5. GrayscaleEffect
//================================================================================
void GrayscaleEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	CreateRootSignature();
	CreatePipelineState();
}
void GrayscaleEffect::CreateRootSignature() {
	rootSignature_ = CreateCommonRootSignature(dxCommon_);
}
void GrayscaleEffect::CreatePipelineState() {
	auto vsBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/FinalBlit.VS.hlsl", L"vs_6_0");
	auto psBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/Grayscale.PS.hlsl", L"ps_6_0");
	assert(vsBlob && psBlob);

	auto gdesc = CreateDefaultPipelineDesc(rootSignature_.Get(), vsBlob.Get(), psBlob.Get());
	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&gdesc, IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr));
}
void GrayscaleEffect::Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) {
	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	cmdList->SetPipelineState(pipelineState_.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);
	cmdList->DrawInstanced(3, 1, 0, 0);
}

//================================================================================
// 6. OutlineEffect
//================================================================================
void OutlineEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	CreateRootSignature();
	CreatePipelineState();
}
void OutlineEffect::CreateRootSignature() {
	rootSignature_ = CreateCommonRootSignature(dxCommon_);
}
void OutlineEffect::CreatePipelineState() {
	auto vsBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/FinalBlit.VS.hlsl", L"vs_6_0");
	auto psBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/Outline.PS.hlsl", L"ps_6_0");
	assert(vsBlob && psBlob);

	auto gdesc = CreateDefaultPipelineDesc(rootSignature_.Get(), vsBlob.Get(), psBlob.Get());
	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&gdesc, IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr));
}
void OutlineEffect::Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) {
	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	cmdList->SetPipelineState(pipelineState_.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);
	cmdList->DrawInstanced(3, 1, 0, 0);
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

	CreateRootSignature();
	CreatePipelineState();
}
void VignetteEffect::SetColor(const Vector4& color) {
	color_ = color;
	if (paramsData_) {
		paramsData_->color = color;
	}
}
void VignetteEffect::CreateRootSignature() {
	// テクスチャ t0 (ピクセル) ＋ 定数バッファ b0 (ピクセル)
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature {};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0; // t0
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[2] = {};
	// Parameter 0: t0 Table
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	// Parameter 1: b0 CBV
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[1].Descriptor.ShaderRegister = 0; // b0

	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0; // s0
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	assert(SUCCEEDED(hr));

	hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}
void VignetteEffect::CreatePipelineState() {
	auto vsBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/FinalBlit.VS.hlsl", L"vs_6_0");
	auto psBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/Vignetting.PS.hlsl", L"ps_6_0");
	assert(vsBlob && psBlob);

	auto gdesc = CreateDefaultPipelineDesc(rootSignature_.Get(), vsBlob.Get(), psBlob.Get());
	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&gdesc, IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr));
}
void VignetteEffect::Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) {
	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	cmdList->SetPipelineState(pipelineState_.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);
	cmdList->SetGraphicsRootConstantBufferView(1, constantBuffer_->GetGPUVirtualAddress()); // b0
	cmdList->DrawInstanced(3, 1, 0, 0);
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

	CreateRootSignature();
	CreatePipelineState();
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
void DissolveEffect::CreateRootSignature() {
	// ディゾルブ専用ルートシグネチャ (画面 t0 + マスク t1 + 定数バッファ b0)
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature {};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descriptorRanges[2] = {};
	// t0 (メイン画面テクスチャ)
	descriptorRanges[0].BaseShaderRegister = 0; // t0
	descriptorRanges[0].NumDescriptors = 1;
	descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// t1 (ディゾルブ用ノイズテクスチャ)
	descriptorRanges[1].BaseShaderRegister = 1; // t1
	descriptorRanges[1].NumDescriptors = 1;
	descriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[3] = {};
	// Parameter 0: t0 Table
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRanges[0];
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;

	// Parameter 1: t1 Table
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[1].DescriptorTable.pDescriptorRanges = &descriptorRanges[1];
	rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;

	// Parameter 2: b0 CBV
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].Descriptor.ShaderRegister = 0; // b0

	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0; // s0
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	assert(SUCCEEDED(hr));

	hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}
void DissolveEffect::CreatePipelineState() {
	auto vsBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/FinalBlit.VS.hlsl", L"vs_6_0");
	auto psBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/Dissolve.PS.hlsl", L"ps_6_0");
	assert(vsBlob && psBlob);

	auto gdesc = CreateDefaultPipelineDesc(rootSignature_.Get(), vsBlob.Get(), psBlob.Get());
	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&gdesc, IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr));
}
void DissolveEffect::Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) {
	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	cmdList->SetPipelineState(pipelineState_.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Parameter 0: Main scene texture
	cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);
	
	// Parameter 1: Noise texture
	D3D12_GPU_DESCRIPTOR_HANDLE noiseHandle = TextureManager::GetInstance()->GetSrvHandleGPU(noiseTextureFilePath_);
	cmdList->SetGraphicsRootDescriptorTable(1, noiseHandle);

	// Parameter 2: Constant buffer
	cmdList->SetGraphicsRootConstantBufferView(2, constantBuffer_->GetGPUVirtualAddress());

	cmdList->DrawInstanced(3, 1, 0, 0);
}
