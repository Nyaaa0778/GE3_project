#pragma once

#include <d3d12.h>
#include <memory>
#include <wrl.h>

class DirectXCommon;

class SpriteRenderer {
public:
	//================================================================================
	// シングルトン
	//================================================================================

	// 唯一のインスタンス取得
	static SpriteRenderer* GetInstance();

	/// <summary>
	/// コンストラクタ
	/// </summary>
	SpriteRenderer() = default;
	/// <summary>
	/// デストラクタ
	/// </summary>
	~SpriteRenderer() = default;

	/// <summary>
	/// 終了
	/// </summary>
	static void Finalize();

	// unique_ptrからの削除を許可
	friend std::default_delete<SpriteRenderer>;

private:
	static std::unique_ptr<SpriteRenderer> instance;

	/// <summary>
	/// コピーコンストラクタ禁止
	/// </summary>
	/// <param name="">コピー元（使用不可）</param>
	SpriteRenderer(SpriteRenderer&) = delete;
	/// <summary>
	/// 代入演算子禁止
	/// </summary>
	/// <param name="">代入元（使用不可）</param>
	/// <returns>このオブジェクトを返す</returns>
	SpriteRenderer& operator=(SpriteRenderer&) = delete;

public:
	//================================================================================
	// Blend Mode
	//================================================================================

	enum class BlendMode {
		kNone,             // なし
		kNormal,           // 通常
		kAdd,              // 加算
		kSubtract,         // 減算
		kMultiply,         // 乗算
		kScreen,           // スクリーン
		kCountOfBlendMode, // カウント用
	};

public:
	//================================================================================
	// 初期化 / 描画設定
	//================================================================================

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(DirectXCommon* dxCommon);

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void SetupCommonRenderState(BlendMode blendMode = BlendMode::kNormal);

private:
	//================================================================================
	// 型エイリアス
	//================================================================================

	// namespace
	template <class InterfaceType>
	using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

private:
	//================================================================================
	// 外部参照
	//================================================================================

	// DirectXCommonのポインタ
	DirectXCommon* dxCommon_ = nullptr;

	//================================================================================
	// GPU リソース
	//================================================================================

	// ルートシグネチャ
	ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	// グラフィックスパイプラインステート (BlendModeごとに保持)
	ComPtr<ID3D12PipelineState> graphicsPipelineStates_[static_cast<size_t>(BlendMode::kCountOfBlendMode)];

private:
	//================================================================================
	// パイプライン構築（RootSignature / PSO）
	//================================================================================

	/// <summary>
	/// ルートシグネチャの生成
	/// </summary>
	void CreateRootSignature();

	/// <summary>
	/// グラフィックスパイプラインの生成
	/// </summary>
	void CreateGraphicsPipeline();

	/// <summary>
	/// 指定したブレンドモードに対応する設定を返す
	/// </summary>
	D3D12_BLEND_DESC MakeBlendDesc(BlendMode mode);
};
