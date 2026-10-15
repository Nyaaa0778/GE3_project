#include "PrimitiveRenderer.h"
#include "DirectXCommon.h"
#include "PipelineStateCache.h"

#include <cassert>
#include <d3d12.h>

std::unique_ptr<PrimitiveRenderer> PrimitiveRenderer::instance = nullptr;

PrimitiveRenderer* PrimitiveRenderer::GetInstance() {
	if (instance == nullptr) {
		instance = std::make_unique<PrimitiveRenderer>();
	}
	return instance.get();
}

void PrimitiveRenderer::Finalize() { instance.reset(); }

void PrimitiveRenderer::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;

	// シェーダコンパイル
	vsBlob_ = dxCommon_->CompileShader(L"resources/shaders/primitive/Primitive.VS.hlsl", L"vs_6_0");
	psBlob_ = dxCommon_->CompileShader(L"resources/shaders/primitive/Primitive.PS.hlsl", L"ps_6_0");

	// InputLayout 設定
	inputElementDescs_[0] = {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
	inputElementDescs_[1] = {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
	inputElementDescs_[2] = {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
}

void PrimitiveRenderer::SetupCommonRenderState(BlendMode blendMode, bool isSolid) {
	auto* psoCache = PipelineStateCache::GetInstance();

	// 描画キーの構築
	PipelineDescKey key{};
	key.vsBlob = vsBlob_.Get();
	key.psBlob = psBlob_.Get();
	key.inputElements = inputElementDescs_;
	key.numInputElements = _countof(inputElementDescs_);
	key.blendMode = blendMode;
	key.cullMode = isSolid ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE;
	key.fillMode = D3D12_FILL_MODE_SOLID;
	key.depthEnable = true;
	key.depthWrite = isSolid;
	key.depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	key.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// キャッシュから PSO を取得
	ID3D12PipelineState* pso = psoCache->GetOrCreate(key);

	auto commandList = dxCommon_->GetCommandList();
	commandList->SetGraphicsRootSignature(psoCache->GetCommonRootSignature());
	commandList->SetPipelineState(pso);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

PrimitiveRenderer::SharedGeometry* PrimitiveRenderer::GetSharedGeometry(const std::string& name) {
	auto it = sharedGeometries_.find(name);
	if (it != sharedGeometries_.end()) {
		return &it->second;
	}
	return nullptr;
}

void PrimitiveRenderer::SetSharedGeometry(const std::string& name, const SharedGeometry& geometry) {
	sharedGeometries_[name] = geometry;
}

