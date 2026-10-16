#pragma once

#include <d3d12.h>
#include <memory>
#include <wrl.h>

#include <dxcapi.h>

#include <BlendMode.h>

class DirectXCommon;

class SkyboxRenderer {
public:
	//================================================================================
	// シングルトン
	//================================================================================

	// 唯一のインスタンス取得
	static SkyboxRenderer* GetInstance();

	/// <summary>
	/// コンストラクタ
	/// </summary>
	SkyboxRenderer() = default;
	/// <summary>
	/// デストラクタ
	/// </summary>
	~SkyboxRenderer() = default;

	/// <summary>
	/// 終了
	/// </summary>
	static void Finalize();

	// unique_ptrからの削除を許可
	friend std::default_delete<SkyboxRenderer>;

private:
	static std::unique_ptr<SkyboxRenderer> instance;

	/// <summary>
	/// コピーコンストラクタ禁止
	/// </summary>
	/// <param name="">コピー元（使用不可）</param>
	SkyboxRenderer(SkyboxRenderer&) = delete;
	/// <summary>
	/// 代入演算子禁止
	/// </summary>
	/// <param name="">代入元（使用不可）</param>
	/// <returns>このオブジェクトを返す</returns>
	SkyboxRenderer& operator=(SkyboxRenderer&) = delete;

public:
	//================================================================================
	// パブリック関数
	//================================================================================

	// 初期化
	void Initialize(DirectXCommon* dxCommon);

	// 描画前共通設定
	void PreDraw();

	// DirectXCommonの取得
	DirectXCommon* GetDxCommon() const { return dxCommon_; }

private:
	//================================================================================
	// 型エイリアス
	//================================================================================

	// namespace
	template <class InterfaceType>
	using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

private:
	//================================================================================
	// メンバ変数
	//================================================================================

	// 共通基盤
	DirectXCommon* dxCommon_ = nullptr;

	ComPtr<IDxcBlob> vsBlob_;
	ComPtr<IDxcBlob> psBlob_;
};