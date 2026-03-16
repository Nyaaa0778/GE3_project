#pragma once

#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <wrl.h>

class ShaderResourceViewManager {
public:
	//================================================================================
	// 定数
	//================================================================================

	// 最大SRV数（最大テクスチャ枚数）
	static const uint32_t kMaxSRVCount;

public:
	//================================================================================
	// シングルトン
	//================================================================================

	// 唯一のインスタンス取得
	static ShaderResourceViewManager* GetInstance();

	/// <summary>
	/// コンストラクタ
	/// </summary>
	ShaderResourceViewManager() = default;
	/// <summary>
	/// デストラクタ
	/// </summary>
	~ShaderResourceViewManager() = default;


	/// <summary>
	/// 終了
	/// </summary>
	static void Finalize();

	// unique_ptrからの削除を許可
	friend std::default_delete<ShaderResourceViewManager>;

private:
	static std::unique_ptr<ShaderResourceViewManager> instance;

	/// <summary>
	/// コピーコンストラクタ禁止
	/// </summary>
	/// <param name="">コピー元（使用不可）</param>
	ShaderResourceViewManager(ShaderResourceViewManager&) = delete;
	/// <summary>
	/// 代入演算子禁止
	/// </summary>
	/// <param name="">代入元（使用不可）</param>
	/// <returns>このオブジェクトを返す</returns>
	ShaderResourceViewManager& operator=(ShaderResourceViewManager&) = delete;

public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// SRV用ディスクリプタインデックスを発行
	/// </summary>
	/// <returns>確保したSRVディスクリプタのインデックス番号</returns>
	uint32_t Allocate();

	/// <summary>
	/// SRV生成（2D）
	/// </summary>
	/// <param name="srvIndex">書き込むSRVインデックス</param>
	/// <param name="pResource">対象のテクスチャリソース</param>
	/// <param name="Format">テクスチャのフォーマット</param>
	/// <param name="MipLevels">ミップマップの数</param>
	void CreateSRVfortexture2D(uint32_t srvIndex, ID3D12Resource* pResource,
		DXGI_FORMAT Format, UINT MipLevels);

	/// <summary>
	/// SRV生成（StructureBuffer）
	/// </summary>
	/// <param name="srvIndex">書き込むSRVインデックス</param>
	/// <param name="pResource">対象のStructureBufferリソース</param>
	/// <param name="numElements">バッファに含まれる要素（データ）の数</param>
	/// <param name="structureByteStride">1要素あたりのサイズ（バイト）</param>
	void CreateSRVforStructureBuffer(uint32_t srvIndex, ID3D12Resource* pResource,
		UINT numElements, UINT structureByteStride);

	/// <summary>
	/// 描画前処理
	/// </summary>
	void BeginDraw();

private:
	//================================================================================
	// 型エイリアス
	//================================================================================

	// namespace
	template <class InterfaceType>
	using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

public:
	//================================================================================
	// Getter
	//================================================================================

	/// <summary>
	/// CPUの取得
	/// </summary>
	/// <param
	/// name="index">取得したいディスクリプタハンドルのインデックス番号</param>
	/// <returns>取得したSRV用のCPUハンドル</returns>
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
	/// <summary>
	/// GPUの取得
	/// </summary>
	/// <param
	/// name="index">取得したいディスクリプタハンドルのインデックス番号</param>
	/// <returns>取得したSRV用のGPUハンドル</returns>
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

	// デスクリプタヒープ
	ID3D12DescriptorHeap* GetDescriptorHeap() const {
		return descriptorHeap_.Get();
	}

	/// <summary>
	/// 新しくSRVを1つ確保可能かどうかを返す
	/// </summary>
	/// <returns>確保可能ならtrue、不可能ならfalse</returns>
	bool CanAllocate() const;

	//================================================================================
	// Setter
	//================================================================================

	/// <summary>
	/// コマンドリストのルートパラメータスロットにSRVデスクリプタをセット
	/// </summary>
	/// <param name="RootParameterIndex">シェーダー側の登録スロット番号</param>
	/// <param name="srvIndex">使用するSRVインデックス</param>
	void SetGraphicsRootDescriptorTable(UINT RootParameterIndex,
		uint32_t srvIndex);

private:
	// デスクリプタサイズ
	uint32_t descriptorSize_;
	// デスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> descriptorHeap_ = nullptr;

	// 次に使用するSRVインデックス
	uint32_t useIndexNext_ = 0;
};
