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
    key.inputLayout = InputLayoutType::kPosition;
    key.blendMode = BlendMode::kNormal;
    key.cullMode = D3D12_CULL_MODE_NONE;
    key.fillMode = D3D12_FILL_MODE_SOLID;
    key.depthEnable = true;
    key.depthWrite = false;
    key.depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    key.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // キャッシュから PSO を取得
    ID3D12PipelineState* pso = psoCache->GetOrCreate(key);

    // ルートシグネチャをセット（重複発行スキップ）
    psoCache->SetGraphicsRootSignature(commandList, psoCache->GetCommonRootSignature());
    // グラフィックスパイプラインステートをセット（重複発行スキップ）
    psoCache->SetGraphicsPipelineState(commandList, pso);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}