#pragma once

#include <d3d12.h>
#include <memory>
#include <wrl.h>

class DirectXCommon;
class Camera;

class Object3dRenderer {
public:
	//================================================================================
	// シングルトン
	//================================================================================

	// 唯一のインスタンス取得
	static Object3dRenderer* GetInstance();

	/// <summary>
	/// コンストラクタ
	/// </summary>
	Object3dRenderer() = default;
	/// <summary>
	/// デストラクタ
	/// </summary>
	~Object3dRenderer() = default;

	/// <summary>
	/// 終了
	/// </summary>
	static void Finalize();

	// unique_ptrからの削除を許可
	friend std::default_delete<Object3dRenderer>;

private:
	static std::unique_ptr<Object3dRenderer> instance;

	/// <summary>
	/// コピーコンストラクタ禁止
	/// </summary>
	/// <param name="">コピー元（使用不可）</param>
	Object3dRenderer(Object3dRenderer&) = delete;
	/// <summary>
	/// 代入演算子禁止
	/// </summary>
	/// <param name="">代入元（使用不可）</param>
	/// <returns>このオブジェクトを返す</returns>
	Object3dRenderer& operator=(Object3dRenderer&) = delete;

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

private:
	BlendMode blendMode_ = BlendMode::kNormal;

public:
	//================================================================================
	// 初期化 / 描画設定
	//================================================================================

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="dxCommon">DirectXCommonのポインタ</param>
	void Initialize(DirectXCommon* dxCommon);

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void SetupCommonRenderState();

	//================================================================================
	// Getter
	//================================================================================

	// DirectXCommon
	DirectXCommon* GetDxCommon() const { return dxCommon_; }
	// デフォルトカメラ
	Camera* GetDefaultCamera() const { return defaultCamera_; }

	// Blend Mode
	BlendMode GetBlendMode() const { return blendMode_; }

	//================================================================================
	// Setter
	//================================================================================

	// デフォルトカメラ
	void SetDefaultCamera(Camera* defaultCamera) {
		defaultCamera_ = defaultCamera;
	}

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
	// デフォルトカメラ
	Camera* defaultCamera_ = nullptr;

	//================================================================================
	// GPU リソース
	//================================================================================

	// ルートシグネチャ
	ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;

	// グラフィックスパイプラインステート
	ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;

private:
	//================================================================================
	// パイプライン構築（RootSignature / PSO）
	//================================================================================

	/// <summary>
	/// ルートシグネチャを作成
	/// </summary>
	void CreateRootSignature();
	/// <summary>
	/// グラフィックスパイプラインの生成
	/// </summary>
	void CreateGraphicsPipeline();

	//================================================================================
	// BlendMode
	//================================================================================

	/// <summary>
	/// 指定したブレンドモードに対応
	/// </summary>
	/// <param name="mode">使いたいBlendMode</param>
	/// <returns>ブレンド設定を格納したD3D12_BLEND_DESC</returns>
	D3D12_BLEND_DESC MakeBlendDesc(BlendMode mode);
};
