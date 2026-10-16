#include "SpriteRenderer.h"

#include "DirectXCommon.h"
#include "PipelineStateCache.h"

//================================================================================
// シングルトン
//================================================================================

std::unique_ptr<SpriteRenderer> SpriteRenderer::instance = nullptr;

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>SpriteRendererの唯一のインスタンス</returns>
SpriteRenderer* SpriteRenderer::GetInstance() {
	if (instance == nullptr)
	{
		instance = std::make_unique<SpriteRenderer>();
	}

	return instance.get();
}

/// <summary>
/// 終了
/// </summary>
void SpriteRenderer::Finalize() { instance.reset(); }

//================================================================================
// 初期化 / 描画設定
//================================================================================

/// <summary>
/// 初期化
/// </summary>
void SpriteRenderer::Initialize(DirectXCommon* dxCommon) {

	// メンバ変数に記録
	dxCommon_ = dxCommon;

	// シェーダコンパイル
	vsBlob_ = dxCommon_->CompileShader(L"resources/shaders/sprite/Sprite.VS.hlsl", L"vs_6_0");
	psBlob_ = dxCommon_->CompileShader(L"resources/shaders/sprite/Sprite.PS.hlsl", L"ps_6_0");
}

/// <summary>
/// 共通描画設定
/// </summary>
void SpriteRenderer::SetupCommonRenderState(BlendMode blendMode) {
	auto* psoCache = PipelineStateCache::GetInstance();

	// 描画キーの構築
	PipelineDescKey key{};
	key.vsBlob = vsBlob_.Get();
	key.psBlob = psBlob_.Get();
	key.inputLayout = InputLayoutType::kPositionTexcoord;
	key.blendMode = blendMode;
	key.cullMode = D3D12_CULL_MODE_NONE;
	key.depthEnable = false;

	// キャッシュから PSO を取得
	ID3D12PipelineState* pso = psoCache->GetOrCreate(key);

	dxCommon_->GetCommandList()->SetGraphicsRootSignature(psoCache->GetCommonRootSignature());
	dxCommon_->GetCommandList()->SetPipelineState(pso);
	// プリミティブトポロジーをセット
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}