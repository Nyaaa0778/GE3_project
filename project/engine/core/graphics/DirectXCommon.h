#pragma once

#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>

#include "../../externals/DirectXTex/DirectXTex.h"

#include <array>
#include <memory>
#include <string>
#include <wrl.h>

class WinApp;

class DirectXCommon
{
public:
	//================================================================================
	// 定数
	//================================================================================

	// SRVの個数
	static const uint32_t kMaxSRVCount;

public:
	//================================================================================
	// シングルトン
	//================================================================================

	// 唯一のインスタンス取得
	static DirectXCommon* GetInstance();

	/// <summary>
	/// コンストラクタ
	/// </summary>
	DirectXCommon() = default;
	/// <summary>
	/// デストラクタ
	/// </summary>
	~DirectXCommon();

	/// <summary>
	/// 終了
	/// </summary>
	static void Finalize();

	// unique_ptrからの削除を許可
	friend std::default_delete<DirectXCommon>;

private:
	static std::unique_ptr<DirectXCommon> instance;

	/// <summary>
	/// コピーコンストラクタ禁止
	/// </summary>
	/// <param name="">コピー元（使用不可）</param>
	DirectXCommon(DirectXCommon&) = delete;
	/// <summary>
	/// 代入演算子禁止
	/// </summary>
	/// <param name="">代入元（使用不可）</param>
	/// <returns>このオブジェクトを返す</returns>
	DirectXCommon& operator=(DirectXCommon&) = delete;

public:
	//================================================================================
	// 初期化 / 更新 / 描画
	//================================================================================

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(WinApp* winApp);

	/// <summary>
	/// RenderTextureの初期化
	/// </summary>
	void InitializeRenderTexture();

	/// <summary>
	/// 描画前処理
	/// </summary>
	void BeginDraw();
	/// <summary>
	/// 描画後処理
	/// </summary>
	void EndDraw();

	/// <summary>
	/// Swapchainに対してImGuiを描画する設定
	/// </summary>
	void PreDrawImGui();

public:
	//================================================================================
	// Getter
	//================================================================================

	// 画面サイズ
	uint32_t GetClientWidth() const;
	uint32_t GetClientHeight() const;

	// デバイス
	ID3D12Device* GetDevice() const { return device_.Get(); }
	// コマンドリスト
	ID3D12GraphicsCommandList* GetCommandList() const {
		return commandList_.Get();
	}

	// バックバッファの数
	UINT GetSwapChainResourceNum() const { return kBackBufferCount; }

	// SRVのデスクリプタヒープ
	ID3D12DescriptorHeap* GetSrvDescriptorHeap() const {
		return descriptorHeapSRV_.Get();
	}
	// SRVのデスクリプタサイズ
	UINT GetSrvDescriptorSize() const { return descriptorSizeSRV_; }

	/// <summary>
	/// SRVの指定番号のCPUデスクリプタハンドルを取得
	/// </summary>
	/// <param name="index">取得したいSRVのインデックス番号</param>
	/// <returns>SRVのCPUディスクリプタハンドル</returns>
	D3D12_CPU_DESCRIPTOR_HANDLE GetSrvCPUDescriptorHandle(uint32_t index);
	/// <summary>
	/// SRVの指定番号のGPUデスクリプタハンドルを取得
	/// </summary>
	/// <param name="index">取得したいSRVのインデックス番号</param>
	/// <returns>SRVのCPUディスクリプタハンドル</returns>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGPUDescriptorHandle(uint32_t index);

	uint32_t GetRenderTextureSrvIndex() const { return srvIndexRenderTexture_; }

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

	// WinAppのポインタ
	WinApp* winApp_ = nullptr;

	//================================================================================
	// DirectX12 コアオブジェクト
	//================================================================================

	// DirectX12デバイス
	ComPtr<ID3D12Device> device_ = nullptr;
	// DXGIファクトリ
	ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;

	//================================================================================
	// コマンド
	//================================================================================

	// コマンドアロケータ
	ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
	// コマンドリスト
	ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;
	// コマンドキュー
	ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;

	//================================================================================
	// スワップチェーン / バックバッファ
	//================================================================================

	// スワップチェーン
	ComPtr<IDXGISwapChain4> swapChain_ = nullptr;

	// バックバッファの枚数
	static const UINT kBackBufferCount = 2;
	// スワップチェーンリソース
	std::array<ComPtr<ID3D12Resource>, kBackBufferCount> swapChainResources_;

	// RTVハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[kBackBufferCount] = {};

	//================================================================================
	// 深度バッファ
	//================================================================================

	// 深度バッファリソース
	ComPtr<ID3D12Resource> depthStencilResource_ = nullptr;

	//================================================================================
	// デスクリプタ
	//================================================================================

	// RTVのデスクリプタサイズ
	uint32_t descriptorSizeRTV_;
	// SRVのデスクリプタサイズ
	uint32_t descriptorSizeSRV_;
	// DSVのデスクリプタサイズ
	uint32_t descriptorSizeDSV_;

	// RTVのデスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> descriptorHeapRTV_;
	// SRVのデスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> descriptorHeapSRV_;
	// DSVのデスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> descriptorHeapDSV_;

	//================================================================================
	// 同期オブジェクト（フェンス）
	//================================================================================

	// フェンス
	ComPtr<ID3D12Fence> fence_ = nullptr;
	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_;

	//================================================================================
	// ビューポート / シザー
	//================================================================================

	// ビューポート矩形
	D3D12_VIEWPORT viewportRect_ {};

	// シザー矩形
	D3D12_RECT scissorRect_ {};

	//================================================================================
	// DXCコンパイラ
	//================================================================================

	// DXCコンパイラ
	IDxcUtils* dxcUtils_ = nullptr;
	IDxcCompiler3* dxcCompiler_ = nullptr;
	IDxcIncludeHandler* includeHandler_ = nullptr;

	//================================================================================
	// RenderTexture
	//================================================================================
	ComPtr<ID3D12Resource> renderTextureResource_ = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleRenderTexture_ {}; // RenderTexture用のRTV
	uint32_t srvIndexRenderTexture_ = 0;                   // RenderTexture用のSRVインデックス

public:
	//================================================================================
	// シェーダ / GPUリソース / デスクリプタ生成
	//================================================================================

	/// <summary>
	/// 指定されたHLSLファイルをコンパイルしてシェーダーバイナリを生成
	/// </summary>
	/// <param name="filePath">コンパイルするHLSLファイルのパス</param>
	/// <param name="profile">コンパイルするシェーダープロファイル</param>
	/// <returns>コンパイル済みシェーダーを格納したIDxcBlob*、失敗した場合はnullptr</returns>
	ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath,
		const wchar_t* profile);

	/// <summary>
	/// Resource作成関数
	/// </summary>
	/// <param name="device"></param>
	/// <param name="sizeInBytes"></param>
	/// <returns></returns>
	ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);

	/// <summary>
	/// テクスチャ用リソースの作成
	/// </summary>
	/// <param name="device"></param>
	/// <param name="metadata"></param>
	/// <returns></returns>
	ComPtr<ID3D12Resource>
		CreateTextureResource(const DirectX::TexMetadata& metadata);

	/// <summary>
	/// 画像データをCPUで書き込み、TextureResourceに転送
	/// </summary>
	/// <param
	/// name="texture">転送先のGPUテクスチャリソース（DEFAULTヒープ想定、事前に
	/// STATE_COPY_DEST）</param> <param
	/// name="mipImages">DirectXTexのScratchImage</param>
	[[nodiscard]]
	ComPtr<ID3D12Resource>
		UploadTextureData(const ComPtr<ID3D12Resource>& texture,
			const DirectX::ScratchImage& mipImages);

	/// <summary>
	/// デスクリプタヒープの生成
	/// </summary>
	/// <param name="heapType">RTV、SRV、DSVなど</param>
	/// <param name="numDescriptors">作成するディスクリプタ（ビュー）の数</param>
	/// <param
	/// name="shaderVisible">GPU（シェーダー）から参照できるようにするかどうか</param>
	/// <returns>生成したデスクリプタヒープ</returns>
	ComPtr<ID3D12DescriptorHeap>
		CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors,
			bool shaderVisible);

private:
	//================================================================================
	// Getter
	//================================================================================

	/// <summary>
	/// CPUの取得
	/// </summary>
	/// <param name="descriptorHeap">RTV、SRV、DSVなど</param>
	/// <param name="descriptorSize">デスクリプタヒープのサイズ</param>
	/// <param
	/// name="index">取得したいディスクリプタハンドルのインデックス番号</param>
	/// <returns>取得したCPUハンドル</returns>
	static D3D12_CPU_DESCRIPTOR_HANDLE
		GetCPUDescriptorHandle(const ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
			uint32_t descriptorSize, uint32_t index);
	/// <summary>
	/// GPUの取得
	/// </summary>
	/// <param name="descriptorHeap"></param>
	/// <param name="descriptorSize"></param>
	/// <param
	/// name="index">取得したいディスクリプタハンドルのインデックス番号</param>
	/// <returns>取得したGPUハンドル</returns>
	static D3D12_GPU_DESCRIPTOR_HANDLE
		GetGPUDescriptorHandle(const ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
			uint32_t descriptorSize, uint32_t index);

private:
	//================================================================================
	// 内部初期化処理
	//================================================================================

	/// <summary>
	/// デバイスの初期化
	/// </summary>
	void InitializeDevice();

	/// <summary>
	/// コマンドの初期化
	/// </summary>
	void InitializeCommand();

	/// <summary>
	/// スワップチェーンの生成
	/// </summary>
	void CreateSwapChain();

	/// <summary>
	/// 深度バッファの生成
	/// </summary>
	/// <returns>生成した深度バッファリソース</returns>
	void CreateDepthBuffer();

	/// <summary>
	/// 各種デスクリプタヒープの生成
	/// </summary>
	void CreateDescriptorHeaps();

	/// <summary>
	/// レンダーターゲットビューの初期化
	/// </summary>
	void InitializeRenderTargetView();

	/// <summary>
	/// 深度ステンシルビューの初期化
	/// </summary>
	void InitializeDepthStencilView();

	/// <summary>
	/// フェンスの生成
	/// </summary>
	void CreateFence();

	/// <summary>
	/// ビューポート矩形の初期化
	/// </summary>
	void InitializeViewportRect();

	/// <summary>
	/// シザー矩形の初期化
	/// </summary>
	void InitializeScissorRect();

	/// <summary>
	/// DXCコンパイラの生成
	/// </summary>
	void CreateDXCCompiler();
};
