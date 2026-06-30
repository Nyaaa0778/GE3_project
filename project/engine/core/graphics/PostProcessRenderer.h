#pragma once

#include <d3d12.h>
#include <memory>
#include <string>
#include <wrl.h>
#include <Vector4.h>
#include <unordered_map>
#include "IPostProcessEffect.h"

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
		kBoxFilter,
		kGaussianFilter,
		kGrayscale,
		kOutline,
		kVignetting,
		kDissolve,
		kRandomNoise,
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
	float GetDissolveThreshold() const;

	void SetDissolveNoiseTexture(const std::string& filePath);
	const std::string& GetDissolveNoiseTexture() const;

	void SetVignetteColor(const Vector4& color);
	Vector4 GetVignetteColor() const;

	/// <summary>
	/// 各種エフェクトを型安全に取得するテンプレートメソッド
	/// </summary>
	template <class T>
	T* GetEffect(PostProcessMode mode) {
		auto it = effects_.find(mode);
		if (it != effects_.end()) {
			return dynamic_cast<T*>(it->second.get());
		}
		return nullptr;
	}

private:
	//================================================================================
	// 外部参照
	//================================================================================
	DirectXCommon* dxCommon_ = nullptr;

	//================================================================================
	// 設定パラメータ
	//================================================================================
	PostProcessMode mode_ = PostProcessMode::kNormal;

	//================================================================================
	// 各エフェクトのインスタンス管理
	//================================================================================
	std::unordered_map<PostProcessMode, std::unique_ptr<IPostProcessEffect>> effects_;
};