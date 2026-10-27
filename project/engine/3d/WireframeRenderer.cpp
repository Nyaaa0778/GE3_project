#include "WireframeRenderer.h"

#include "DirectXCommon.h"
#include "PipelineStateCache.h"

#include <cassert>
#include <d3d12.h>

//================================================================================
// シングルトン
//================================================================================

std::unique_ptr<WireframeRenderer> WireframeRenderer::instance = nullptr;

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>WireframeRendererの唯一のインスタンス</returns>
WireframeRenderer* WireframeRenderer::GetInstance() {
	if (instance == nullptr)
	{
		instance = std::make_unique<WireframeRenderer>();
	}

	return instance.get();
}

/// <summary>
/// 終了
/// </summary>
void WireframeRenderer::Finalize() { instance.reset(); }

//================================================================================
// 初期化 / 描画設定
//================================================================================

/// <summary>
/// 初期化
/// </summary>
/// <param name="dxCommon">DirectXCommonのポインタ</param>
void WireframeRenderer::Initialize(DirectXCommon* dxCommon) {
	// 引数で受け取ってメンバ変数に記録する
	dxCommon_ = dxCommon;

	// シェーダコンパイル
	vsBlob_ = dxCommon_->CompileShader(L"resources/shaders/wireframe/Wireframe.VS.hlsl", L"vs_6_0");
	psBlob_ = dxCommon_->CompileShader(L"resources/shaders/wireframe/Wireframe.PS.hlsl", L"ps_6_0");
}

/// <summary>
/// 共通描画設定
/// </summary>
void WireframeRenderer::SetupCommonRenderState(bool depthEnable, BlendMode blendMode) {
	auto* psoCache = PipelineStateCache::GetInstance();

	// 描画キーの構築
	PipelineDescKey key{};
	key.vsBlob = vsBlob_.Get();
	key.psBlob = psBlob_.Get();
	key.inputLayout = InputLayoutType::kPosition;
	key.blendMode = blendMode;
	key.cullMode = D3D12_CULL_MODE_NONE;
	key.fillMode = D3D12_FILL_MODE_SOLID;
	key.depthEnable = depthEnable;
	key.depthWrite = depthEnable;
	key.depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	key.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

	// キャッシュから PSO を取得
	ID3D12PipelineState* pso = psoCache->GetOrCreate(key);

	// ルートシグネチャをセット（重複発行スキップ）
	psoCache->SetGraphicsRootSignature(dxCommon_->GetCommandList(), psoCache->GetCommonRootSignature());
	// グラフィックスパイプラインステートをセット（重複発行スキップ）
	psoCache->SetGraphicsPipelineState(dxCommon_->GetCommandList(), pso);
	// プリミティブトポロジーをセット
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_LINELIST);
}