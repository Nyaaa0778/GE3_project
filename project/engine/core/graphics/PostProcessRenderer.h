#pragma once

#include <d3d12.h>
#include <memory>
#include <wrl.h>

class DirectXCommon;

class PostProcessRenderer
{
public:
	//================================================================================
	// シングルトン
	//================================================================================

	// 唯一のインスタンス取得
	static PostProcessRenderer* GetInstance();

	/// <summary>
	/// コンストラクタ
	/// </summary>
	PostProcessRenderer() = default;
	/// <summary>
	/// デストラクタ
	/// </summary>
	~PostProcessRenderer() = default;

	/// <summary>
	/// 終了
	/// </summary>
	static void Finalize();

	// unique_ptrからの削除を許可
	friend std::default_delete<PostProcessRenderer>;

private:
	static std::unique_ptr<PostProcessRenderer> instance;

	/// <summary>
	/// コピーコンストラクタ禁止
	/// </summary>
	PostProcessRenderer(PostProcessRenderer&) = delete;
	/// <summary>
	/// 代入演算子禁止
	/// </summary>
	PostProcessRenderer& operator=(PostProcessRenderer&) = delete;

public:
	//================================================================================
	// 初期化 / 描画
	//================================================================================

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(DirectXCommon* dxCommon);

	/// <summary>
	/// 描画（フルスクリーントライアングルによるテクスチャコピー）
	/// </summary>
	/// <param name="srvHandle">画面に貼り付けたいテクスチャのSRVハンドル</param>
	void Draw(D3D12_GPU_DESCRIPTOR_HANDLE srvHandle);

private:
	//================================================================================
	// 型エイリアス
	//================================================================================
	template <class InterfaceType>
	using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

	//================================================================================
	// 外部参照
	//================================================================================
	DirectXCommon* dxCommon_ = nullptr;

	//================================================================================
	// GPU リソース
	//================================================================================
	ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;

	//================================================================================
	// パイプライン構築
	//================================================================================
	void CreateRootSignature();
	void CreateGraphicsPipeline();
};