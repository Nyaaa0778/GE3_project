#pragma once

#include "../../externals/DirectXTex/DirectXTex.h"

#include <d3d12.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <wrl.h>

class DirectXCommon;
class ShaderResourceViewManager;

class TextureManager {
public:
	//================================================================================
	// シングルトン
	//================================================================================

	// 唯一のインスタンス取得
	static TextureManager* GetInstance();

	/// <summary>
	/// コンストラクタ
	/// </summary>
	TextureManager() = default;
	/// <summary>
	/// デストラクタ
	/// </summary>
	~TextureManager() = default;


	/// <summary>
	/// 終了
	/// </summary>
	static void Finalize();

	// unique_ptrからの削除を許可
	friend std::default_delete<TextureManager>;

private:
	static std::unique_ptr<TextureManager> instance;

	/// <summary>
	/// コピーコンストラクタ禁止
	/// </summary>
	/// <param name="">コピー元（使用不可）</param>
	TextureManager(TextureManager&) = delete;
	/// <summary>
	/// 代入演算子禁止
	/// </summary>
	/// <param name="">代入元（使用不可）</param>
	/// <returns>このオブジェクトを返す</returns>
	TextureManager& operator=(TextureManager&) = delete;

public:
	//================================================================================
	// 初期化
	//================================================================================

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(DirectXCommon* dxCommon,
		ShaderResourceViewManager* srvManager);

public:
	//================================================================================
	// テクスチャ読み込み / 中間リソース解放
	//================================================================================

	/// <summary>
	/// 画像ファイルを読み込みテクスチャに変換
	/// </summary>
	/// <param name="filePath"></param>
	/// <returns></returns>
	void LoadTexture(const std::string& filePath);

	/// <summary>
	/// 中間リソースの解放
	/// </summary>
	void ReleaseIntermediateResources();

public:
	//================================================================================
	// Getter
	//================================================================================

	/// <summary>
	/// SRVインデックスの取得
	/// </summary>
	/// <param name="filePath">検索対象となるテクスチャのファイルパス</param>
	/// <returns>見つかったテクスチャのSRVインデックス、
	/// 該当するテクスチャが存在しない場合はassertによりプログラムを停止する</returns>
	uint32_t GetTextureIndexByFilePath(const std::string& filePath);
	/// <summary>
	/// SRVハンドルの取得
	/// </summary>
	/// <param name="filePath">テクスチャのファイルパス</param>
	/// <returns>指定インデックスのSRVのGPUデスクリプタハンドル</returns>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);

	/// <summary>
	/// メタデータを取得
	/// </summary>
	/// <param name="filePath">テクスチャのファイルパス</param>
	/// <returns>指定テクスチャの幅・高さ・フォーマットなどのメタデータ</returns>
	const DirectX::TexMetadata& GetMetaData(const std::string& filePath);

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

	// srvManagerのポインタ
	ShaderResourceViewManager* srvManager_ = nullptr;

private:
	//================================================================================
	// 内部構造体
	//================================================================================

	// テクスチャ一枚分のデータ
	struct TextureData {
		DirectX::TexMetadata metadata;
		ComPtr<ID3D12Resource> resource;
		uint32_t srvIndex;
		ComPtr<ID3D12Resource> intermediateResource;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
	};

	//================================================================================
	// テクスチャ管理データ（読み込み済み）
	//================================================================================

	// テクスチャデータ
	std::unordered_map<std::string, TextureData> textureDatas_;

	// SRVインデックスの開始番号
	static uint32_t kSRVIndexTop;
};
