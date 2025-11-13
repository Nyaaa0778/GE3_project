#include "DirectXCommon.h"
#include "Logger.h"
#include "StringUtility.h"

using namespace Logger;
using namespace StringUtility;

#include <cassert>
#include <format>

#include "../../externals/DirectXTex/d3dx12.h"
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")

/// <summary>
/// 初期化
/// </summary>
void DirectXCommon::Initialize(WinApp *winApp) {
  // NULL検出
  assert(winApp);
  // メンバ変数に記録
  winApp_ = winApp;

  // デバイスの初期化
  InitializeDevice();
  // コマンドの初期化
  InitializeCommand();
  // スワップチェーンの生成
  CreateSwapChain();
  // 深度バッファの生成
  CreateDepthBuffer();
  // 各種でスクリプタヒープの生成
  CreateDescriptorHeaps();
  // RTVの初期化
  InitializeRenderTargetView();
  // 深度ステンシルビューの初期化
  InitializeDepthStencilView();
  // フェンスの生成
  CreateFence();
  // ビューポート矩形の初期化
  InitializeViewportRect();
  // シザー矩形の初期化
  InitializeScissorRect();
  // DXCコンパイラの生成
  CreateDXCCompiler();
  // ImGuiの初期化
  InitializeImGui();
}

/// <summary>
/// 描画前処理
/// </summary>
void DirectXCommon::BeginDraw() {
  // バックバッファの番号取得
  UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

  // リソースバリアで書き込み可能に変更 (Present → RenderTarget)
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  commandList_->ResourceBarrier(1, &barrier);

  // 描画先のRTVとDSVを指定する
  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
      descriptorHeapDSV_->GetCPUDescriptorHandleForHeapStart();
  commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false,
                                   &dsvHandle);

  // 画面全体の深度をクリア
  commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f,
                                      0, 0, nullptr);

  // 画面全体の色をクリア
  const float clearColor[4] = {0.1f, 0.25f, 0.5f, 1.0f};
  commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor,
                                      0, nullptr);

  // SRV用のデスクリプタヒープを指定する
  ID3D12DescriptorHeap *descriptorHeaps[] = {descriptorHeapSRV_.Get()};
  commandList_->SetDescriptorHeaps(1, descriptorHeaps);

  // ビューポート領域の設定
  commandList_->RSSetViewports(1, &viewportRect_);

  // シザー矩形の設定
  commandList_->RSSetScissorRects(1, &scissorRect_);
}
/// <summary>
/// 描画後処理
/// </summary>
void DirectXCommon::EndDraw() {
  // バックバッファの番号取得
  UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

  // リソースバリアを表示状態に変更（RenderTarget → Present）
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  commandList_->ResourceBarrier(1, &barrier);

  // コマンドリストをクローズ
  HRESULT hr = commandList_->Close();
  assert(SUCCEEDED(hr));

  // GPUにコマンドリストを実行させる
  ID3D12CommandList *commandLists[] = {commandList_.Get()};
  commandQueue_->ExecuteCommandLists(1, commandLists);

  // GPU画面の交換を通知（Present）
  swapChain_->Present(1, 0);

  // Fence値を更新
  fenceValue_++;

  // GPU完了シグナルを送信
  commandQueue_->Signal(fence_.Get(), fenceValue_);

  // Fence完了待ち
  if (fence_->GetCompletedValue() < fenceValue_) {
    fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
    WaitForSingleObject(fenceEvent_, INFINITE);
  }

  // コマンドアロケータとコマンドリストのリセット
  hr = commandAllocator_->Reset();
  assert(SUCCEEDED(hr));
  hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
  assert(SUCCEEDED(hr));
}

/// <summary>
/// デバイスの初期化
/// </summary>
void DirectXCommon::InitializeDevice() {
  // DXGIファクトリーの生成

  // HRESULTはWindows系のエラーコードであり、
  // 関数が成功したかどうかをSUCCEEDEDマクロで判定できる
  HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
  // 初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、
  // どうにもできない場合が多いのでassertにしておく
  assert(SUCCEEDED(hr));

  // 使用するアダプタ用の変数、最初にnullptrを入れる
  ComPtr<IDXGIAdapter4> useAdapter = nullptr;
  // いい順にアダプタを読む
  for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(
                       i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                       IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND;
       i++) {
    // アダプターの情報を取得する
    DXGI_ADAPTER_DESC3 adapterDesc{};
    hr = useAdapter->GetDesc3(&adapterDesc);
    assert(SUCCEEDED(hr));
    // ソフトウェアアダプタでなければ採用
    if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
      // 採用したアダプタの情報をログに出力(wstring)
      Log(ConvertString(
          std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
      break;
    }
    useAdapter = nullptr;
  }
  // 適切なアダプタが見つからなかったら、起動できない
  assert(useAdapter != nullptr);

  // 機能レベルとログの出力用の文字列
  D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0};
  const char *featureLevelString[] = {"12.2", "12.1", "12.0"};
  // 高い順に生成できるか？
  for (size_t i = 0; i < _countof(featureLevels); ++i) {
    // 採用したアダプターでデバイスを作成
    hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i],
                           IID_PPV_ARGS(&device_));
    // 指定した機能レベルでデバイスが生成できたかを確認
    if (SUCCEEDED(hr)) {
      // 生成できたのでログ出力を行ってループを抜ける
      Log(std::format("FeatureLevel : {}\n", featureLevelString[i]));
      break;
    }
  }

  // デバイスの生成がうまくいかなかったので起動できない
  assert(device_ != nullptr);
  // 初期化完了用のログを出す
  Log("Complete create D3D12Device!!!\n");

#ifdef _DEBUG
  ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
  if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
    // 危険なエラー発生時に停止
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
    // エラー発生時に停止
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
    // 警告時に停止
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

    // 抑制するメッセージID
    D3D12_MESSAGE_ID denyIds[] = {
        D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE};
    // 抑制するレベル
    D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
    D3D12_INFO_QUEUE_FILTER filter{};
    filter.DenyList.NumIDs = _countof(denyIds);
    filter.DenyList.pIDList = denyIds;
    filter.DenyList.NumSeverities = _countof(severities);
    filter.DenyList.pSeverityList = severities;
    // 指定したメッセージの表示を抑制する
    infoQueue->PushStorageFilter(&filter);

    // 解放
    infoQueue->Release();
  }
#endif
}

/// <summary>
/// コマンドの初期化
/// </summary>
void DirectXCommon::InitializeCommand() {
  // コマンドアロケータを生成する
  HRESULT hr = device_->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
  // コマンドアロケータの生成がうまくいかなかったので起動できない
  assert(SUCCEEDED(hr));

  // コマンドリストを生成
  hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                  commandAllocator_.Get(), nullptr,
                                  IID_PPV_ARGS(&commandList_));
  // コマンドリストの生成がうまくいかなかったので起動できない
  assert(SUCCEEDED(hr));

  // コマンドキューを生成
  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
  hr = device_->CreateCommandQueue(&commandQueueDesc,
                                   IID_PPV_ARGS(&commandQueue_));
  // コマンドキューの生成が上手くいかなかったので起動できない
  assert(SUCCEEDED(hr));
}

/// <summary>
/// スワップチェーンの生成
/// </summary>
void DirectXCommon::CreateSwapChain() {
  // スワップチェーンを生成
  swapChainDesc_.Width =
      winApp_->kClientWidth; // 画面の幅、ウィンドウのクライアント領域と同じ
  swapChainDesc_.Height =
      winApp_->kClientHeight; // 画面の高さ、ウィンドウのクライアント領域と同じ
  swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色の形式
  swapChainDesc_.SampleDesc.Count = 1; // マルチサンプルしない(ギザギザ)
  swapChainDesc_.BufferUsage =
      DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画ターゲットとして利用
  swapChainDesc_.BufferCount = 2;      // ダブルバッファ
  swapChainDesc_.SwapEffect =
      DXGI_SWAP_EFFECT_FLIP_DISCARD; // モニタにうつしたら、中身を放棄

  // コマンドキュー、ウィンドウハンドル、設定を渡して生成
  HRESULT hr = dxgiFactory_->CreateSwapChainForHwnd(
      commandQueue_.Get(), winApp_->GetHwnd(), &swapChainDesc_, nullptr,
      nullptr, reinterpret_cast<IDXGISwapChain1 **>(swapChain_.GetAddressOf()));
  assert(SUCCEEDED(hr));
}

/// <summary>
/// 深度バッファの生成
/// </summary>
DirectXCommon::ComPtr<ID3D12Resource> DirectXCommon::CreateDepthBuffer() {
  // 生成するResourceの設定
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Width = winApp_->kClientWidth;   // Textureの幅
  resourceDesc.Height = winApp_->kClientHeight; // Textureの高さ
  resourceDesc.MipLevels = 1;                   // mipmapの数
  resourceDesc.DepthOrArraySize = 1;            // 奥行 or 配列Textureの配列数
  resourceDesc.Format =
      DXGI_FORMAT_D24_UNORM_S8_UINT; // DepthStencilとして利用可能なフォーマット
  resourceDesc.SampleDesc.Count = 1; // サンプリングカウント、1固定
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 二次元
  resourceDesc.Flags =
      D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知

  // 利用するHeapの設定
  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

  // 深度値のクリア設定
  D3D12_CLEAR_VALUE depthClearValue{};
  depthClearValue.DepthStencil.Depth = 1.0f; // 最大値(一番遠い状態)
  depthClearValue.Format =
      DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット、Resourceと合わせる

  // Resourceの生成
  HRESULT hr = device_->CreateCommittedResource(
      &heapProperties,                  // Heapの設定
      D3D12_HEAP_FLAG_NONE,             // Heaoの特殊な設定
      &resourceDesc,                    // Resourceの設定
      D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込む状態にする
      &depthClearValue,                 // Clear最適地
      IID_PPV_ARGS(
          &depthStencilResource_) // 作成するResourceポインタへのポインタ
  );
  assert(SUCCEEDED(hr));

  return depthStencilResource_;
}

/// <summary>
/// デスクリプタヒープの生成
/// </summary>
/// <param name="device">D3D12デバイス</param>
/// <param name="heapType">RTV、SRV、DSVなど</param>
/// <param name="numDescriptors">作成するディスクリプタ（ビュー）の数</param>
/// <param
/// name="shaderVisible">GPU（シェーダー）から参照できるようにするかどうか</param>
/// <returns>生成したデスクリプタヒープ</returns>
DirectXCommon::ComPtr<ID3D12DescriptorHeap>
DirectXCommon::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                    UINT numDescriptors, bool shaderVisible) {
  ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
  D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
  descriptorHeapDesc.Type = heapType; // レンダーターゲットビュー用
  descriptorHeapDesc.NumDescriptors =
      numDescriptors; // ダブルバッファ用に2つ、多い文にはOK
  descriptorHeapDesc.Flags = shaderVisible
                                 ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                 : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  // ディスクリプタヒープが作れなかったので起動できない
  HRESULT hr = device_->CreateDescriptorHeap(&descriptorHeapDesc,
                                             IID_PPV_ARGS(&descriptorHeap));
  return descriptorHeap;
}
/// <summary>
/// 各種デスクリプタヒープの生成
/// </summary>
void DirectXCommon::CreateDescriptorHeaps() {
  // DescriptorSizeを取得
  descriptorSizeSRV_ = device_->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  descriptorSizeRTV_ =
      device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  descriptorSizeDSV_ =
      device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

  // RTV用のヒープ(ディスクリプタの数は2)、RTVはShader内で触るものではないため、ShaderVisibleはfalse
  descriptorHeapRTV_ =
      CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
  // SRV用のヒープ(ディスクリプタの数は128)、RTVはShader内で触るものなので、ShaderVisibleはtrue
  descriptorHeapSRV_ =
      CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
  // DSV用のヒープディスクリプタの数は1、ShaderVisibleはfalse
  descriptorHeapDSV_ =
      CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
}

/// <summary>
/// CPUの取得
/// </summary>
/// <param name="descriptorHeap">RTV、SRV、DSVなど</param>
/// <param name="descriptorSize">デスクリプタヒープのサイズ</param>
/// <param
/// name="index">取得したいディスクリプタハンドルのインデックス番号</param>
/// <returns>取得したCPUハンドル</returns>
D3D12_CPU_DESCRIPTOR_HANDLE
DirectXCommon::GetCPUDescriptorHandle(
    const ComPtr<ID3D12DescriptorHeap> &descriptorHeap, uint32_t descriptorSize,
    uint32_t index) {
  D3D12_CPU_DESCRIPTOR_HANDLE handle =
      descriptorHeap->GetCPUDescriptorHandleForHeapStart();
  handle.ptr += (descriptorSize * index);

  return handle;
}
/// <summary>
/// GPUの取得
/// </summary>
/// <param name="descriptorHeap"></param>
/// <param name="descriptorSize"></param>
/// <param
/// name="index">取得したいディスクリプタハンドルのインデックス番号</param>
/// <returns>取得したGPUハンドル</returns>
D3D12_GPU_DESCRIPTOR_HANDLE
DirectXCommon::GetGPUDescriptorHandle(
    const ComPtr<ID3D12DescriptorHeap> &descriptorHeap, uint32_t descriptorSize,
    uint32_t index) {
  D3D12_GPU_DESCRIPTOR_HANDLE handle =
      descriptorHeap->GetGPUDescriptorHandleForHeapStart();
  handle.ptr += (descriptorSize * index);

  return handle;
}
/// <summary>
/// SRVの指定番号のCPUデスクリプタハンドルを取得
/// </summary>
/// <param name="index">取得したいSRVのインデックス番号</param>
/// <returns>SRVのCPUディスクリプタハンドル</returns>
D3D12_CPU_DESCRIPTOR_HANDLE
DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index) {
  return GetCPUDescriptorHandle(descriptorHeapSRV_, descriptorSizeSRV_, index);
}
/// <summary>
/// SRVの指定番号のGPUデスクリプタハンドルを取得
/// </summary>
/// <param name="index">取得したいSRVのインデックス番号</param>
/// <returns>SRVのCPUディスクリプタハンドル</returns>
D3D12_GPU_DESCRIPTOR_HANDLE
DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index) {
  return GetGPUDescriptorHandle(descriptorHeapSRV_, descriptorSizeSRV_, index);
}
/// <summary>
/// レンダーターゲットビューの初期化
/// </summary>
void DirectXCommon::InitializeRenderTargetView() {
  // スワップチェーンからバックバッファを引っ張ってくる
  for (UINT i = 0; i < kBackBufferCount; ++i) {
    HRESULT hr =
        swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources_[i]));
    assert(SUCCEEDED(hr));
  }

  // RTVの設定
  rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 出力結果をSRGBに変換
  rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2Dテクスチャ

  // 先頭ハンドル（参考: 直接使ってもOK、実際の作成はfor内でindexを進める）
  D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle =
      descriptorHeapRTV_->GetCPUDescriptorHandleForHeapStart();

  // ★要素数ぶん回してRTVを作成（資料の for）
  for (UINT i = 0; i < kBackBufferCount; ++i) {
    // i番目のRTVハンドルを計算して保存
    rtvHandles_[i] =
        GetCPUDescriptorHandle(descriptorHeapRTV_, descriptorSizeRTV_, i);
    // i番目のバックバッファに対するRTVを作成
    device_->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc_,
                                    rtvHandles_[i]);
  }
}

/// <summary>
/// 深度ステンシルビューの初期化
/// </summary>
void DirectXCommon::InitializeDepthStencilView() {
  // DSVの設定
  D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
  dsvDesc.Format =
      DXGI_FORMAT_D24_UNORM_S8_UINT; // FOrmat、基本的にはResourceに合わせる
  dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2dTexture
  // DSVHeapの先頭にDSVをつくる
  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
      GetCPUDescriptorHandle(descriptorHeapDSV_, descriptorSizeDSV_, 0);

  device_->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc,
                                  dsvHandle);

  // DepthStencilStateの設定
  D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
  // Depthの機能を有効化
  depthStencilDesc.DepthEnable = true;
  // 書き込む
  depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  // 比較関数はLessEqual、近ければ描画される
  depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
}

/// <summary>
/// フェンスの生成
/// </summary>
void DirectXCommon::CreateFence() {
  // 初期化0でFenceを作る
  HRESULT hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE,
                                    IID_PPV_ARGS(&fence_));
  assert(SUCCEEDED(hr));

  // FenceのSignalを持つためのイベントを作成
  fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
  assert(fenceEvent_ != nullptr);
}

/// <summary>
/// ビューポート矩形の初期化
/// </summary>
void DirectXCommon::InitializeViewportRect() {
  // ビューポート
  // クライアント領域のサイズと一緒にして画面全体に表示
  viewportRect_.Width = winApp_->kClientWidth;
  viewportRect_.Height = winApp_->kClientHeight;
  viewportRect_.TopLeftX = 0;
  viewportRect_.TopLeftY = 0;
  viewportRect_.MinDepth = 0.0f;
  viewportRect_.MaxDepth = 1.0f;
}

/// <summary>
/// シザー矩形の初期化
/// </summary>
void DirectXCommon::InitializeScissorRect() {
  // シザー矩形
  // 基本的にビューポートと同じ矩形が構成されるようにする
  scissorRect_.left = 0;
  scissorRect_.right = WinApp::kClientWidth;
  scissorRect_.top = 0;
  scissorRect_.bottom = WinApp::kClientHeight;
}

/// <summary>
/// DXCコンパイラの生成
/// </summary>
void DirectXCommon::CreateDXCCompiler() {
  // dxCompilerを初期化
  HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
  assert(SUCCEEDED(hr));
  hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
  assert(SUCCEEDED(hr));

  // 現時点ではincludeしないが、includeに対応するために設定を行う
  hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
  assert(SUCCEEDED(hr));
}

/// <summary>
/// ImGuiの初期化
/// </summary>
void DirectXCommon::InitializeImGui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplWin32_Init(winApp_->GetHwnd());
  ImGui_ImplDX12_Init(device_.Get(), swapChainDesc_.BufferCount,
                      rtvDesc_.Format, descriptorHeapSRV_.Get(),
                      descriptorHeapSRV_->GetCPUDescriptorHandleForHeapStart(),
                      descriptorHeapSRV_->GetGPUDescriptorHandleForHeapStart());
}

/// <summary>
/// 指定されたHLSLファイルをコンパイルしてシェーダーバイナリを生成
/// </summary>
/// <param name="filePath">コンパイルするHLSLファイルのパス</param>
/// <param name="profile">コンパイルするシェーダープロファイル</param>
/// <returns>コンパイル済みシェーダーを格納したIDxcBlob*、失敗した場合はnullptr</returns>
DirectXCommon::ComPtr<IDxcBlob>
DirectXCommon::CompileShader(const std::wstring &filePath,
                             const wchar_t *profile) {

  // シェーダーをコンパイルする旨をログに出す
  Log(ConvertString(std::format(L"Begin CompileShader,path:{},profile:{}\n",
                                filePath, profile)));
  // hlslファイルを読み込む
  IDxcBlobEncoding *shaderSource = nullptr;
  HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
  // 読めなかったら止める
  assert(SUCCEEDED(hr));
  // 読み込んだファイルの内容を設定する
  DxcBuffer shaderSourceBuffer;
  shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
  shaderSourceBuffer.Size = shaderSource->GetBufferSize();
  shaderSourceBuffer.Encoding = DXC_CP_UTF8;

  LPCWSTR arguments[] = {
      filePath.c_str(), // コンパイル対象のhlslファイル名
      L"-E",
      L"main", // エントリーポイントの指定、基本的にmain以外にはしない
      L"-T",
      profile, // ShaderProfileの設定
      L"-Zi",
      L"-Qembed_debug", // デバッグ用の情報を埋め込む
      L"-Od",           // 最適化を外しておく
      L"-Zpr",          // メモリレイアウトは行優先
  };
  // 実際にShaderをコンパイルする
  IDxcResult *shaderResult = nullptr;
  hr = dxcCompiler_->Compile(&shaderSourceBuffer, // 読み込んだファイル
                             arguments,           // コンパイルオプション
                             _countof(arguments), // コンパイルオプションの数
                             includeHandler_, // incluldeが含まれたものいろいろ
                             IID_PPV_ARGS(&shaderResult));
  // コンパイルエラーではなくdxcが起動できないなど致命的な状況のとき
  assert(SUCCEEDED(hr));

  // 警告、エラーが出てきたらログに出して停止
  IDxcBlobUtf8 *shaderError = nullptr;
  shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
  if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
    Log(shaderError->GetStringPointer());
    // 警告、エラーはだめ
    assert(false);
  }

  // こんなピル結果から実行用のバイナリ部分を取得
  IDxcBlob *shaderBlob = nullptr;
  hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob),
                               nullptr);
  assert(SUCCEEDED(hr));
  // 成功したログを出す
  Log(ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n",
                                filePath, profile)));
  // もう使わないリソースを解放
  shaderSource->Release();
  shaderResult->Release();
  // 実行用のバイナリを返却
  return shaderBlob;
}

/// <summary>
/// リソースの作成
/// </summary>
/// <param name="sizeInBytes">作成するバッファのサイズ（バイト単位）</param>
/// <returns>生成されたID3D12ResourceのComPtr、リソース作成に失敗した場合はnullptr</returns>
DirectXCommon::ComPtr<ID3D12Resource>
DirectXCommon::CreateBufferResource(size_t sizeInBytes) {
  // 頂点リソース用のヒープの設定
  D3D12_HEAP_PROPERTIES uploadHeapProperties{};
  uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // UploadHeapを使用
  // 頂点リソースの設定
  D3D12_RESOURCE_DESC vertexResourceDesc{};
  // バッファリソース、テクスチャの場合はまた別の設定をする
  vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  vertexResourceDesc.Width = sizeInBytes;
  // バッファの場合は以下は1にする
  vertexResourceDesc.Height = 1;
  vertexResourceDesc.DepthOrArraySize = 1;
  vertexResourceDesc.MipLevels = 1;
  vertexResourceDesc.SampleDesc.Count = 1;
  // バッファの場合は以下にする
  vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  // 実際に頂点リソースを作る
  ComPtr<ID3D12Resource> vertexResource = nullptr;
  HRESULT hr = device_->CreateCommittedResource(
      &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&vertexResource));
  assert(SUCCEEDED(hr));

  return vertexResource;
}

/// <summary>
/// テクスチャ用リソースの作成
/// </summary>
/// <param name="metadata">DirectXTexのTexMetadata構造体</param>
/// <returns>作成されたID3D12ResourceのComPtr、生成に失敗した場合はnullptr</returns>
DirectXCommon::ComPtr<ID3D12Resource>
DirectXCommon::CreateTextureResource(const DirectX::TexMetadata &metadata) {
  // metadataを基にResourceを設定
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Width = UINT(metadata.width);           // Textureの幅
  resourceDesc.Height = UINT(metadata.height);         // Textureの高さ
  resourceDesc.MipLevels = UINT16(metadata.mipLevels); // mipmapの数
  resourceDesc.DepthOrArraySize =
      UINT(metadata.arraySize);          // 奥行き or 配列Textureの配列数
  resourceDesc.Format = metadata.format; // TextureのFormat
  resourceDesc.SampleDesc.Count = 1;     // サンプリングカウント、1固定
  resourceDesc.Dimension =
      D3D12_RESOURCE_DIMENSION(metadata.dimension); // Textureの次元数

  // 利用するHeapの設定
  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // 細かい設定を行う
  // heapProperties.CPUPageProperty =
  //     D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; //
  //     writeBackポリシーでCPUアクセス可能
  // heapProperties.MemoryPoolPreference =
  //     D3D12_MEMORY_POOL_L0; // プロセッサの近くに配置

  // Resourceを生成
  ComPtr<ID3D12Resource> resource = nullptr;
  HRESULT hr = device_->CreateCommittedResource(
      &heapProperties,                // Heapの設定
      D3D12_HEAP_FLAG_NONE,           // Heapの特殊な設定
      &resourceDesc,                  // Resourceの設定
      D3D12_RESOURCE_STATE_COPY_DEST, // データ転送される設定
      nullptr,                        // Clear最適地、使わないのでnullptr
      IID_PPV_ARGS(&resource)         // 作成するResourceポインタへのポインタ
  );
  assert(SUCCEEDED(hr));
  return resource;
}

/// <summary>
/// 画像データをCPUで書き込み、TextureResourceに転送
/// </summary>
/// <param
/// name="texture">転送先のGPUテクスチャリソース（DEFAULTヒープ想定、事前に
/// STATE_COPY_DEST）</param> <param
/// name="mipImages">DirectXTexのScratchImage</param>
void DirectXCommon::UploadTextureData(const ComPtr<ID3D12Resource> &texture,
                                      const DirectX::ScratchImage &mipImages) {
  // 読み込んだデータからサブリソース配列を作成
  std::vector<D3D12_SUBRESOURCE_DATA> subresources;
  DirectX::PrepareUpload(device_.Get(), mipImages.GetImages(),
                         mipImages.GetImageCount(), mipImages.GetMetadata(),
                         subresources);
  // 必要なサイズを計算
  uint64_t intermediateSize =
      GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));
  // 計算したサイズでintermediateResourceを作成
  intermediateResource_ = CreateBufferResource(intermediateSize);

  // intermediateResourceにSubresourceのデータを書き込み、textureに転送
  UpdateSubresources(commandList_.Get(), texture.Get(),
                     intermediateResource_.Get(), 0, 0,
                     UINT(subresources.size()), subresources.data());

  // Textureへの転送後は利用できるように、STATE_COPY_DESTからSTATE_GENERIC_READへResourceStateを変更
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = texture.Get();
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
  commandList_->ResourceBarrier(1, &barrier);
}

/// <summary>
/// 画像ファイルを読み込みテクスチャに変換
/// </summary>
/// <param name="filePath"></param>
/// <returns></returns>
DirectX::ScratchImage DirectXCommon::LoadTexture(const std::string &filePath) {
  // テクスチャファイルを読んでプログラムで扱えるようにする
  DirectX::ScratchImage image{};
  std::wstring filePathW = ConvertString(filePath); // Wはワイド文字列を意味する
  HRESULT hr = DirectX::LoadFromWICFile(
      filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
  assert(SUCCEEDED(hr));

  // ミニマップの作成
  // mipMap: 元画像より小さなテクスチャ群
  DirectX::ScratchImage mipImages{};
  hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                image.GetMetadata(), DirectX::TEX_FILTER_SRGB,
                                0, mipImages);
  assert(SUCCEEDED(hr));

  // ミニマップ付きのデータを返す
  return mipImages;
}