#pragma once

#include <d3d12.h>
#include <memory>
#include <string>
#include <wrl.h>
#include <Vector4.h>

class DirectXCommon;

class PostProcessRenderer
{
public:
	//================================================================================
	// 描画モード
	//================================================================================
	enum class PostProcessMode {
		kNormal,
		kRadialBlur,
		kDissolve,
	};

	//================================================================================
	// ディゾルブパラメータ
	//================================================================================
	struct DissolveParams {
		float threshold;
		float edgeWidth;
		float padding[2];
		Vector4 edgeColor;
	};

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

public:
	//================================================================================
	// Getter / Setter
	//================================================================================
	void SetMode(PostProcessMode mode) { mode_ = mode; }
	PostProcessMode GetMode() const { return mode_; }

	void SetDissolveThreshold(float threshold);
	float GetDissolveThreshold() const { return dissolveThreshold_; }

	void SetDissolveNoiseTexture(const std::string& filePath);
	const std::string& GetDissolveNoiseTexture() const { return dissolveNoiseTextureFilePath_; }

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
	// ルートシグネチャ
	ComPtr<ID3D12RootSignature> rootSignatureNormal_ = nullptr;
	ComPtr<ID3D12RootSignature> rootSignatureDissolve_ = nullptr;

	// パイプラインステート
	ComPtr<ID3D12PipelineState> pipelineStateNormal_ = nullptr;
	ComPtr<ID3D12PipelineState> pipelineStateRadialBlur_ = nullptr;
	ComPtr<ID3D12PipelineState> pipelineStateDissolve_ = nullptr;

	// 定数バッファ
	ComPtr<ID3D12Resource> constantBufferDissolve_ = nullptr;
	DissolveParams* dissolveParamsData_ = nullptr;

	//================================================================================
	// 設定パラメータ
	//================================================================================
	PostProcessMode mode_ = PostProcessMode::kNormal;
	float dissolveThreshold_ = 0.0f;
	std::string dissolveNoiseTextureFilePath_ = "resources/sprites/noise0.png";

	//================================================================================
	// パイプライン構築
	//================================================================================
	void CreateRootSignature();
	void CreateGraphicsPipeline();
};