#include "SkyboxRenderer.h"

#include "DirectXCommon.h"
#include "ShaderResourceViewManager.h"
#include "PipelineStateCache.h"
#include <BlendMode.h>

#include <cassert>

std::unique_ptr<SkyboxRenderer> SkyboxRenderer::instance = nullptr;

//================================================================================
// シングルトン管理
//================================================================================

SkyboxRenderer* SkyboxRenderer::GetInstance() {
    if (instance == nullptr) {
        instance = std::make_unique<SkyboxRenderer>();
    }
    return instance.get();
}

void SkyboxRenderer::Finalize() {
    instance.reset();
}

//================================================================================
// パブリック関数
//================================================================================

void SkyboxRenderer::Initialize(DirectXCommon* dxCommon) {
    assert(dxCommon);
    dxCommon_ = dxCommon;

    // シェーダコンパイル
    vsBlob_ = dxCommon_->CompileShader(L"resources/shaders/skybox/Skybox.VS.hlsl", L"vs_6_0");
    psBlob_ = dxCommon_->CompileShader(L"resources/shaders/skybox/Skybox.PS.hlsl", L"ps_6_0");

    // InputLayout 設定
    inputElementDescs_[0] = {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
}

void SkyboxRenderer::PreDraw() {
    auto commandList = dxCommon_->GetCommandList();

    // ディスクリプタヒープの設定
    ID3D12DescriptorHeap* descriptorHeaps[] = {
        ShaderResourceViewManager::GetInstance()->GetDescriptorHeap()
    };
    commandList->SetDescriptorHeaps(1, descriptorHeaps);

    auto* psoCache = PipelineStateCache::GetInstance();

    // 描画キーの構築
    PipelineDescKey key{};
    key.vsBlob = vsBlob_.Get();
    key.psBlob = psBlob_.Get();
    key.inputElements = inputElementDescs_;
    key.numInputElements = _countof(inputElementDescs_);
    key.blendMode = BlendMode::kNormal;
    key.cullMode = D3D12_CULL_MODE_NONE;
    key.fillMode = D3D12_FILL_MODE_SOLID;
    key.depthEnable = true;
    key.depthWrite = false;
    key.depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    key.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // キャッシュから PSO を取得
    ID3D12PipelineState* pso = psoCache->GetOrCreate(key);

    // ルートシグネチャをセット
    commandList->SetGraphicsRootSignature(psoCache->GetCommonRootSignature());
    // グラフィックスパイプラインステートをセット
    commandList->SetPipelineState(pso);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}