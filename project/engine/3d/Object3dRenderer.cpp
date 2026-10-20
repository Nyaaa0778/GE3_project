#include "Object3dRenderer.h"

#include "DirectXCommon.h"
#include "PipelineStateCache.h"

#include <cassert>
#include <d3d12.h>

//================================================================================
// シングルトン
//================================================================================

std::unique_ptr<Object3dRenderer> Object3dRenderer::instance = nullptr;

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>Object3dRendererの唯一のインスタンス</returns>
Object3dRenderer* Object3dRenderer::GetInstance() {
	if (instance == nullptr) {
		instance = std::make_unique<Object3dRenderer>();
	}

	return instance.get();
}

/// <summary>
/// 終了
/// </summary>
void Object3dRenderer::Finalize() { instance.reset(); }

//================================================================================
// 初期化 / 描画設定
//================================================================================

/// <summary>
/// 初期化
/// </summary>
/// <param name="dxCommon">DirectXCommonのポインタ</param>
void Object3dRenderer::Initialize(DirectXCommon* dxCommon) {
	// 引数で受け取ってメンバ変数に記録する
	dxCommon_ = dxCommon;

	// シェーダコンパイル
	vsBlob_ = dxCommon_->CompileShader(L"resources/shaders/object3d/Object3d.VS.hlsl", L"vs_6_0");
	psBlob_ = dxCommon_->CompileShader(L"resources/shaders/object3d/Object3d.PS.hlsl", L"ps_6_0");
}

/// <summary>
/// 共通描画設定
/// </summary>
void Object3dRenderer::SetupCommonRenderState(BlendMode blendMode) {

	auto* psoCache = PipelineStateCache::GetInstance();

	// 描画キーの構築
	PipelineDescKey key{};
	key.vsBlob = vsBlob_.Get();
	key.psBlob = psBlob_.Get();
	key.inputLayout = InputLayoutType::kPositionTexcoordNormal;
	key.blendMode = blendMode;
	key.cullMode = D3D12_CULL_MODE_BACK;
	key.fillMode = D3D12_FILL_MODE_SOLID;
	key.depthEnable = true;
	key.depthWrite = true;

	// キャッシュから PSO を取得
	ID3D12PipelineState* pso = psoCache->GetOrCreate(key);

	// ルートシグネチャをセット（重複発行スキップ）
	psoCache->SetGraphicsRootSignature(dxCommon_->GetCommandList(), psoCache->GetCommonRootSignature());
	// グラフィックスパイプラインステートをセット（重複発行スキップ）
	psoCache->SetGraphicsPipelineState(dxCommon_->GetCommandList(), pso);
	// プリミティブトポロジーをセット
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}