#include "PostProcessRenderer.h"
#include "DirectXCommon.h"
#include "TextureManager.h"
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

	// ディゾルブ専用定数バッファの作成
	constantBufferDissolve_ = dxCommon_->CreateBufferResource(sizeof(DissolveParams));
	constantBufferDissolve_->Map(0, nullptr, reinterpret_cast<void**>(&dissolveParamsData_));
	
	// 初期パラメータを設定
	dissolveParamsData_->threshold = 0.0f;
	dissolveParamsData_->edgeWidth = 0.04f;
	dissolveParamsData_->edgeColor = {1.0f, 0.4f, 0.0f, 1.0f}; // オレンジ色の発光

	// デフォルトノイズテクスチャをロード
	TextureManager::GetInstance()->LoadTexture(dissolveNoiseTextureFilePath_);

	CreateGraphicsPipeline();
}

void PostProcessRenderer::SetDissolveThreshold(float threshold) {
	dissolveThreshold_ = threshold;
	if (dissolveParamsData_) {
		dissolveParamsData_->threshold = threshold;
	}
}

void PostProcessRenderer::SetDissolveNoiseTexture(const std::string& filePath) {
	TextureManager::GetInstance()->LoadTexture(filePath);
	dissolveNoiseTextureFilePath_ = filePath;
}

void PostProcessRenderer::CreateRootSignature() {
	// ----------------------------------------------------
	// 1. 通常・ブラー用ルートシグネチャ (画面テクスチャ t0 のみ)
	// ----------------------------------------------------
	{
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

		ComPtr<ID3DBlob> signatureBlob = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		assert(SUCCEEDED(hr));

		hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignatureNormal_));
		assert(SUCCEEDED(hr));
	}

	// ----------------------------------------------------
	// 2. ディゾルブ専用ルートシグネチャ (画面 t0 + マスク t1 + 定数バッファ b0)
	// ----------------------------------------------------
	{
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

		ComPtr<ID3DBlob> signatureBlob = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		assert(SUCCEEDED(hr));

		hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignatureDissolve_));
		assert(SUCCEEDED(hr));
	}
}

void PostProcessRenderer::CreateGraphicsPipeline() {
	CreateRootSignature();

	// InputLayoutは利用しない
	D3D12_INPUT_LAYOUT_DESC inputLayOutDesc {};
	inputLayOutDesc.pInputElementDescs = nullptr;
	inputLayOutDesc.NumElements = 0;

	D3D12_RASTERIZER_DESC rasterizerDesc {};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// DepthStencilState自体の無効化
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc {};
	depthStencilDesc.DepthEnable = false; 
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// ブレンド設定 (無効化)
	D3D12_BLEND_DESC blendDesc {};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	// Vertex Shaderは共通
	ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/FinalBlit.VS.hlsl", L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	// ----------------------------------------------------
	// 1. Normal モード用 PSO
	// ----------------------------------------------------
	{
		ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/FinalBlit.PS.hlsl", L"ps_6_0");
		assert(pixelShaderBlob != nullptr);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC gdesc {};
		gdesc.pRootSignature = rootSignatureNormal_.Get();
		gdesc.InputLayout = inputLayOutDesc;
		gdesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()};
		gdesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};
		gdesc.BlendState = blendDesc;
		gdesc.RasterizerState = rasterizerDesc;
		gdesc.DepthStencilState = depthStencilDesc;
		gdesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		gdesc.NumRenderTargets = 1;
		gdesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		gdesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		gdesc.SampleDesc.Count = 1;
		gdesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&gdesc, IID_PPV_ARGS(&pipelineStateNormal_));
		assert(SUCCEEDED(hr));
	}

	// ----------------------------------------------------
	// 2. RadialBlur モード用 PSO
	// ----------------------------------------------------
	{
		ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/RadialBlur.PS.hlsl", L"ps_6_0");
		assert(pixelShaderBlob != nullptr);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC gdesc {};
		gdesc.pRootSignature = rootSignatureNormal_.Get();
		gdesc.InputLayout = inputLayOutDesc;
		gdesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()};
		gdesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};
		gdesc.BlendState = blendDesc;
		gdesc.RasterizerState = rasterizerDesc;
		gdesc.DepthStencilState = depthStencilDesc;
		gdesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		gdesc.NumRenderTargets = 1;
		gdesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		gdesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		gdesc.SampleDesc.Count = 1;
		gdesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&gdesc, IID_PPV_ARGS(&pipelineStateRadialBlur_));
		assert(SUCCEEDED(hr));
	}

	// ----------------------------------------------------
	// 3. Dissolve モード用 PSO
	// ----------------------------------------------------
	{
		ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->CompileShader(L"resources/shaders/finalBlit/Dissolve.PS.hlsl", L"ps_6_0");
		assert(pixelShaderBlob != nullptr);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC gdesc {};
		gdesc.pRootSignature = rootSignatureDissolve_.Get();
		gdesc.InputLayout = inputLayOutDesc;
		gdesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()};
		gdesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};
		gdesc.BlendState = blendDesc;
		gdesc.RasterizerState = rasterizerDesc;
		gdesc.DepthStencilState = depthStencilDesc;
		gdesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		gdesc.NumRenderTargets = 1;
		gdesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		gdesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		gdesc.SampleDesc.Count = 1;
		gdesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&gdesc, IID_PPV_ARGS(&pipelineStateDissolve_));
		assert(SUCCEEDED(hr));
	}
}

void PostProcessRenderer::Draw(D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) {
	ID3D12GraphicsCommandList* cmdList = dxCommon_->GetCommandList();

	switch (mode_) {
	case PostProcessMode::kNormal:
		cmdList->SetGraphicsRootSignature(rootSignatureNormal_.Get());
		cmdList->SetPipelineState(pipelineStateNormal_.Get());
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);
		break;

	case PostProcessMode::kRadialBlur:
		cmdList->SetGraphicsRootSignature(rootSignatureNormal_.Get());
		cmdList->SetPipelineState(pipelineStateRadialBlur_.Get());
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);
		break;

	case PostProcessMode::kDissolve:
		cmdList->SetGraphicsRootSignature(rootSignatureDissolve_.Get());
		cmdList->SetPipelineState(pipelineStateDissolve_.Get());
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
		// Parameter 0: Main scene texture
		cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);
		
		// Parameter 1: Noise texture
		{
			D3D12_GPU_DESCRIPTOR_HANDLE noiseHandle = TextureManager::GetInstance()->GetSrvHandleGPU(dissolveNoiseTextureFilePath_);
			cmdList->SetGraphicsRootDescriptorTable(1, noiseHandle);
		}

		// Parameter 2: Constant buffer
		cmdList->SetGraphicsRootConstantBufferView(2, constantBufferDissolve_->GetGPUVirtualAddress());
		break;
	}

	// 頂点バッファをセットせず、3頂点分の描画を命令（フルスクリーントライアングル）
	cmdList->DrawInstanced(3, 1, 0, 0);
}