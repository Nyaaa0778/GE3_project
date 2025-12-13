#include "ShaderResourceViewManager.h"
#include "DirectXCommon.h"

#include <cassert>

const uint32_t ShaderResourceViewManager::kMaxSRVCount = 512;

/// <summary>
/// 初期化
/// </summary>
/// <param name="dxCommon">DirectXCommonのポインタ</param>
void ShaderResourceViewManager::Initialize(DirectXCommon *dxCommon) {
  // 引数を受け取ってメンバ変数に記録する
  dxCommon_ = dxCommon;

  // デスクリプタヒープの生成
  descriptorHeap_ = dxCommon_->CreateDescriptorHeap(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);
  // デスクリプタ1個分のサイズを取得
  descriptorSize_ = dxCommon_->GetDevice()->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

uint32_t ShaderResourceViewManager::Allocate() {
  // 上限チェック
  assert(useIndexNext_ < kMaxSRVCount);

  // returnする番号を一番上に一旦記録
  int index = useIndexNext_;
  // 次回のために番号を1進める
  useIndexNext_++;
  // 上で記録した番号をreturn
  return index;
}

/// <summary>
/// SRV生成（2D）
/// </summary>
/// <param name="srvIndex">書き込むSRVインデックス</param>
/// <param name="pResource">対象のテクスチャリソース</param>
/// <param name="Format">テクスチャのフォーマット</param>
/// <param name="MipLevels">ミップマップの数</param>
void ShaderResourceViewManager::CreateSRVfortexture2D(uint32_t srvIndex,
                                                      ID3D12Resource *pResource,
                                                      DXGI_FORMAT Format,
                                                      UINT MipLevels) {
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  // srvDescの各項目を埋める
  srvDesc.Format = Format;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.Texture2D.MipLevels = MipLevels;

  dxCommon_->GetDevice()->CreateShaderResourceView(
      pResource, &srvDesc, GetCPUDescriptorHandle(srvIndex));
}
/// <summary>
/// SRV生成（StructureBuffer）
/// </summary>
/// <param name="srvIndex">書き込むSRVインデックス</param>
/// <param name="pResource">対象のStructureBufferリソース</param>
/// <param name="numElements">バッファに含まれる要素（データ）の数</param>
/// <param name="structureByteStride">1要素あたりのサイズ（バイト）</param>
void ShaderResourceViewManager::CreateSRVforStructureBuffer(
    uint32_t srvIndex, ID3D12Resource *pResource, UINT numElements,
    UINT structureByteStride) {
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  // srvDesc の各項目を埋める
  srvDesc.Format = DXGI_FORMAT_UNKNOWN; // StructuredBuffer は UNKNOWN
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER; // Buffer として見る
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

  srvDesc.Buffer.FirstElement = 0;                          // 先頭から
  srvDesc.Buffer.NumElements = numElements;                 // 要素数
  srvDesc.Buffer.StructureByteStride = structureByteStride; // 1要素のバイト数
  srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

  dxCommon_->GetDevice()->CreateShaderResourceView(
      pResource, &srvDesc, GetCPUDescriptorHandle(srvIndex));
}

/// <summary>
/// 描画前処理
/// </summary>
void ShaderResourceViewManager::BeginDraw() {
  ID3D12DescriptorHeap *descriptorHeaps[] = {descriptorHeap_.Get()};
  dxCommon_->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);
}

/// <summary>
/// コマンドリストのルートパラメータスロットにSRVデスクリプタをセット
/// </summary>
/// <param name="RootParameterIndex">シェーダー側の登録スロット番号</param>
/// <param name="srvIndex">使用するSRVインデックス</param>
void ShaderResourceViewManager::SetGraphicsRootDescriptorTable(
    UINT RootParameterIndex, uint32_t srvIndex) {
  dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(
      RootParameterIndex, GetGPUDescriptorHandle(srvIndex));
}

//================================================================================
// Getter
//================================================================================

/// <summary>
/// CPUの取得
/// </summary>
/// <param
/// name="index">取得したいディスクリプタハンドルのインデックス番号</param>
/// <returns>取得したSRV用のCPUハンドル</returns>
D3D12_CPU_DESCRIPTOR_HANDLE
ShaderResourceViewManager::GetCPUDescriptorHandle(uint32_t index) {
  D3D12_CPU_DESCRIPTOR_HANDLE handleCPU =
      descriptorHeap_->GetCPUDescriptorHandleForHeapStart();

  handleCPU.ptr += (descriptorSize_ * index);
  return handleCPU;
}

/// <summary>
/// GPUの取得
/// </summary>
/// <param
/// name="index">取得したいディスクリプタハンドルのインデックス番号</param>
/// <returns>取得したSRV用のGPUハンドル</returns>
D3D12_GPU_DESCRIPTOR_HANDLE
ShaderResourceViewManager::GetGPUDescriptorHandle(uint32_t index) {
  D3D12_GPU_DESCRIPTOR_HANDLE handleGPU =
      descriptorHeap_->GetGPUDescriptorHandleForHeapStart();

  handleGPU.ptr += (descriptorSize_ * index);
  return handleGPU;
}

/// <summary>
/// 新しくSRVを1つ確保可能かどうかを返す
/// </summary>
/// <returns>確保可能ならtrue、不可能ならfalse</returns>
bool ShaderResourceViewManager::CanAllocate() const {
  return useIndexNext_ < kMaxSRVCount;
}
