#pragma once
#include "WinApp.h"
#include"FixFPS.h"

#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>

#include "../../externals/DirectXTex/DirectXTex.h"
#include <array>
#include <string>
#include <wrl.h>

class DirectXCommon {
public:
  /// <summary>
  /// デストラクタ
  /// </summary>
  ~DirectXCommon();

  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(WinApp *winApp);

  /// <summary>
  /// 描画前処理
  /// </summary>
  void BeginDraw();
  /// <summary>
  /// 描画後処理
  /// </summary>
  void EndDraw();

  /// <summary>
  /// SRVの指定番号のCPUデスクリプタハンドルを取得
  /// </summary>
  /// <param name="index">取得したいSRVのインデックス番号</param>
  /// <returns>SRVのCPUディスクリプタハンドル</returns>
  D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
  /// <summary>
  /// SRVの指定番号のGPUデスクリプタハンドルを取得
  /// </summary>
  /// <param name="index">取得したいSRVのインデックス番号</param>
  /// <returns>SRVのCPUディスクリプタハンドル</returns>
  D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

public:
  /// <summary>
  /// Getter
  /// </summary>
  ID3D12Device *GetDevice() const { return device_.Get(); }
  ID3D12GraphicsCommandList *GetCommandList() const {
    return commandList_.Get();
  }
  ID3D12DescriptorHeap *GetSRVHeap() const { return descriptorHeapSRV_.Get(); }
  UINT GetSRVDescriptorSize() const { return descriptorSizeSRV_; }

private:
  // namespace
  template <class InterfaceType>
  using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

  WinApp *winApp_ = nullptr;

  // DirectX12デバイス
  ComPtr<ID3D12Device> device_ = nullptr;
  // DXGIファクトリ
  ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;

  // コマンドアロケータ
  ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
  // コマンドリスト
  ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;
  // コマンドキュー
  ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;

  // スワップチェーン
  ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc_{};

  // 深度バッファリソース
  ComPtr<ID3D12Resource> depthStencilResource_ = nullptr;

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

  // バックバッファの枚数
  static const UINT kBackBufferCount = 2;
  // スワップチェーンリソース
  std::array<ComPtr<ID3D12Resource>, kBackBufferCount> swapChainResources_;
  D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_{};
  // RTVハンドル
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[kBackBufferCount] = {};

  // フェンス
  ComPtr<ID3D12Fence> fence_ = nullptr;
  uint64_t fenceValue_ = 0;
  HANDLE fenceEvent_;

  // ビューポート矩形
  D3D12_VIEWPORT viewportRect_{};

  // シザー矩形
  D3D12_RECT scissorRect_{};

  // DXCコンパイラ
  IDxcUtils *dxcUtils_ = nullptr;
  IDxcCompiler3 *dxcCompiler_ = nullptr;
  IDxcIncludeHandler *includeHandler_ = nullptr;

  // CPUからGPUへテクスチャデータを転送するための一時バッファ
  ComPtr<ID3D12Resource> intermediateResource_ = nullptr;

  //FPS固定
  FixFPS fixFps_;

public:
  /// <summary>
  /// 指定されたHLSLファイルをコンパイルしてシェーダーバイナリを生成
  /// </summary>
  /// <param name="filePath">コンパイルするHLSLファイルのパス</param>
  /// <param name="profile">コンパイルするシェーダープロファイル</param>
  /// <returns>コンパイル済みシェーダーを格納したIDxcBlob*、失敗した場合はnullptr</returns>
  ComPtr<IDxcBlob> CompileShader(const std::wstring &filePath,
                                 const wchar_t *profile);

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
  CreateTextureResource(const DirectX::TexMetadata &metadata);

  /// <summary>
  /// 画像データをCPUで書き込み、TextureResourceに転送
  /// </summary>
  /// <param
  /// name="texture">転送先のGPUテクスチャリソース（DEFAULTヒープ想定、事前に
  /// STATE_COPY_DEST）</param> <param
  /// name="mipImages">DirectXTexのScratchImage</param>
  void UploadTextureData(const ComPtr<ID3D12Resource> &texture,
                         const DirectX::ScratchImage &mipImages);

  /// <summary>
  /// 画像ファイルを読み込みテクスチャに変換
  /// </summary>
  /// <param name="filePath"></param>
  /// <returns></returns>
  static DirectX::ScratchImage LoadTexture(const std::string &filePath);

private:
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
  ComPtr<ID3D12Resource> CreateDepthBuffer();
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
  /// <summary>
  /// 各種デスクリプタヒープの生成
  /// </summary>
  void CreateDescriptorHeaps();

  /// <summary>
  /// CPUの取得
  /// </summary>
  /// <param name="descriptorHeap">RTV、SRV、DSVなど</param>
  /// <param name="descriptorSize">デスクリプタヒープのサイズ</param>
  /// <param
  /// name="index">取得したいディスクリプタハンドルのインデックス番号</param>
  /// <returns>取得したCPUハンドル</returns>
  static D3D12_CPU_DESCRIPTOR_HANDLE
  GetCPUDescriptorHandle(const ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
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
  GetGPUDescriptorHandle(const ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
                         uint32_t descriptorSize, uint32_t index);
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

  /// <summary>
  /// ImGuiの初期化
  /// </summary>
  void InitializeImGui();
};
