#include "TextureManager.h"
#include "DirectXCommon.h"
#include "ShaderResourceViewManager.h"
#include "StringUtility.h"

using namespace StringUtility;

TextureManager *TextureManager::instance = nullptr;

// ImGuiで0番を使用するため、1番から使用
uint32_t TextureManager::kSRVIndexTop = 1;

//================================================================================
// シングルトン管理 / 初期化・終了
//================================================================================

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>TextureManager の唯一のインスタンス</returns>
TextureManager *TextureManager::GetInstance() {
  if (instance == nullptr) {
    instance = new TextureManager;
  }

  return instance;
}

/// <summary>
/// 初期化
/// </summary>
/// <param name="dxCommon">DirectXCommonのポインタ</param>
/// /// <param name="srvManager">SrvManagerのポインタ</param>
void TextureManager::Initialize(DirectXCommon *dxCommon,
                                ShaderResourceViewManager *srvManager) {
  dxCommon_ = dxCommon;
  srvManager_ = srvManager;

  // SRVの数と同数
  textureDatas_.reserve(dxCommon_->kMaxSRVCount);
}

/// <summary>
/// 終了
/// </summary>
void TextureManager::Shutdown() {
  delete instance;
  instance = nullptr;
}

//================================================================================
// テクスチャ読み込み / 中間リソース解放
//================================================================================

/// <summary>
/// 画像ファイルを読み込みテクスチャに変換
/// </summary>
/// <param name="filePath"></param>
/// <returns></returns>
void TextureManager::LoadTexture(const std::string &filePath) {

  // 読み込み済みテクスチャを検索、読み込み済みなら早期return
  if (textureDatas_.contains(filePath)) {
    return;
  }

  TextureData &textureData = textureDatas_[filePath];

  // テクスチャ枚数上限チェック
  assert(srvManager_->CanAllocate());

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

  textureData.metadata = mipImages.GetMetadata();
  textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);

  textureData.srvIndex = srvManager_->Allocate();
  textureData.srvHandleCPU =
      srvManager_->GetCPUDescriptorHandle(textureData.srvIndex);
  textureData.srvHandleGPU =
      srvManager_->GetGPUDescriptorHandle(textureData.srvIndex);

  srvManager_->CreateSRVfortexture2D(
      textureData.srvIndex, textureData.resource.Get(),
      textureData.metadata.format,
      static_cast<UINT>(textureData.metadata.mipLevels));

  //// テクスチャデータの要素番号をSRVのインデックスにする
  // uint32_t srvIndex =
  //     static_cast<uint32_t>(textureDatas_.size() - 1 + kSRVIndexTop);

  // textureData.srvHandleCPU = dxCommon_->GetSrvCPUDescriptorHandle(srvIndex);
  // textureData.srvHandleGPU = dxCommon_->GetSrvGPUDescriptorHandle(srvIndex);

  /*D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = textureData.metadata.format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels =
      static_cast<UINT>(textureData.metadata.mipLevels);

  dxCommon_->GetDevice()->CreateShaderResourceView(
      textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);*/

  // 転送用に生成した中間リソースをテクスチャデータ構造体に格納
  textureData.intermediateResource =
      dxCommon_->UploadTextureData(textureData.resource, mipImages);
}

/// <summary>
/// 中間リソースの解放
/// </summary>
void TextureManager::ReleaseIntermediateResources() {
  for (std::pair<const std::string, TextureData> &pair : textureDatas_) {

    // 2番目（value側）を取り出す
    TextureData &textureData = pair.second;

    // 中間リソースを解放
    textureData.intermediateResource.Reset();
  }
}

//================================================================================
// Getter
//================================================================================

/// <summary>
/// SRVインデックスの取得
/// </summary>
/// <param name="filePath">検索対象となるテクスチャのファイルパス</param>
/// <returns>見つかったテクスチャのSRVインデックス、
/// 該当するテクスチャが存在しない場合はassertによりプログラムを停止する</returns>
uint32_t
TextureManager::GetTextureIndexByFilePath(const std::string &filePath) {
  // map からテクスチャを検索
  std::unordered_map<std::string, TextureData>::const_iterator it =
      textureDatas_.find(filePath);

  // 存在しなければ停止
  assert(it != textureDatas_.end());

  // value の中にある SRV index を返す
  return it->second.srvIndex;
}

/// <summary>
/// SRVハンドルの取得
/// </summary>
/// <param name="filePath">テクスチャのファイルパス</param>
/// <returns>指定インデックスのSRVのGPUデスクリプタハンドル</returns>
D3D12_GPU_DESCRIPTOR_HANDLE
TextureManager::GetSrvHandlGPU(const std::string &filePath) {
  // map からテクスチャを検索
  std::unordered_map<std::string, TextureData>::iterator it =
      textureDatas_.find(filePath);

  // 見つからなかったら停止
  assert(it != textureDatas_.end());

  // テクスチャデータ（value）を取得
  TextureData &textureData = it->second;

  // そのテクスチャの GPU SRV ハンドルを返す
  return textureData.srvHandleGPU;
}

/// <summary>
/// メタデータを取得
/// </summary>
/// <param name="filePath">テクスチャのファイルパス</param>
/// <returns>指定テクスチャの幅・高さ・フォーマットなどのメタデータ</returns>
const DirectX::TexMetadata &
TextureManager::GetMetaData(const std::string &filePath) {
  // キーでテクスチャを検索
  std::unordered_map<std::string, TextureData>::iterator it =
      textureDatas_.find(filePath);

  // 見つからなかったら停止
  assert(it != textureDatas_.end());

  // テクスチャデータを取得してメタデータを返す
  return it->second.metadata;
}
