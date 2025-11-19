#include "TextureManager.h"
#include <DirectXCommon.h>
#include <StringUtility.h>
using namespace StringUtility;

TextureManager *TextureManager::instance = nullptr;

// ImGuiで0番を使用するため、1番から使用
uint32_t TextureManager::kSRVIndexTop = 1;

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
void TextureManager::Initialize(DirectXCommon *dxCommon) {
  dxCommon_ = dxCommon;

  // SRVの数と同数
  textureDatas_.reserve(dxCommon_->kMaxSRVCount);
}

/// <summary>
/// 終了
/// </summary>
void TextureManager::Finalize() {
  delete instance;
  instance = nullptr;
}

/// <summary>
/// 画像ファイルを読み込みテクスチャに変換
/// </summary>
/// <param name="filePath"></param>
/// <returns></returns>
void TextureManager::LoadTexture(const std::string &filePath) {

  // 読み込み済みテクスチャを検索
  auto it = std::find_if(textureDatas_.begin(), textureDatas_.end(),
                         [&](TextureData &textureData) {
                           return textureData.filePath == filePath;
                         });

  // 読み込み済みなら早期return
  if (it != textureDatas_.end()) {
    return;
  }

  // テクスチャ枚数上限チェック
  assert(textureDatas_.size() + kSRVIndexTop < dxCommon_->kMaxSRVCount);

  // テクスチャデータを追加
  textureDatas_.resize(textureDatas_.size() + 1);
  // 追加したテクスチャデータの参照を取得する
  TextureData &textureData = textureDatas_.back();

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

  textureData.filePath = filePath;
  textureData.metadata = mipImages.GetMetadata();
  textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);

  // テクスチャデータの要素番号をSRVのインデックスにする
  uint32_t srvIndex =
      static_cast<uint32_t>(textureDatas_.size() - 1 + kSRVIndexTop);

  textureData.srvHandleCPU = dxCommon_->GetSrvCPUDescriptorHandle(srvIndex);
  textureData.srvHandleGPU = dxCommon_->GetSrvGPUDescriptorHandle(srvIndex);

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = textureData.metadata.format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels =
      static_cast<UINT>(textureData.metadata.mipLevels);

  dxCommon_->GetDevice()->CreateShaderResourceView(
      textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);

  // 転送用に生成した中間リソースをテクスチャデータ構造体に格納
  textureData.intermediateResource =
      dxCommon_->UploadTextureData(textureData.resource, mipImages);
}

/// <summary>
/// 中間リソースの解放
/// </summary>
void TextureManager::ReleaseIntermediateResources() {
  for (TextureData &textureData : textureDatas_) {
    textureData.intermediateResource.Reset();
  }
}
/// <summary>
/// SRVインデックスの取得
/// </summary>
/// <param name="filePath">検索対象となるテクスチャのファイルパス</param>
/// <returns>見つかったテクスチャのSRVインデックス、
/// 該当するテクスチャが存在しない場合はassertによりプログラムを停止する</returns>
uint32_t TextureManager::GetSrvIndexByFilePath(const std::string &filePath) {
  auto it = std::find_if(textureDatas_.begin(), textureDatas_.end(),
                         [&](TextureData &textureData) {
                           return textureData.filePath == filePath;
                         });

  if (it != textureDatas_.end()) {
    // 読み込み済みなら要素番号を返す
    uint32_t textureIndex =
        static_cast<uint32_t>(std::distance(textureDatas_.begin(), it));
    return textureIndex;
  }

  assert(0);

  return 0;
}

/// <summary>
/// SRVハンドルの取得
/// </summary>
/// <param name="textureIndex">SRVヒープ内のテクスチャインデックス</param>
/// <returns>指定インデックスのSRVのGPUデスクリプタハンドル</returns>
D3D12_GPU_DESCRIPTOR_HANDLE
TextureManager::GetSrvHandlGPU(uint32_t textureIndex) {
  // 範囲外指定違反チェック（テクスチャ番号が正常範囲内である）
  assert(textureIndex < textureDatas_.size());

  // テクスチャデータの参照を取得
  TextureData &textureData = textureDatas_[textureIndex];

  // そのテクスチャに対応するGPUのSRVハンドルを返す
  return textureData.srvHandleGPU;
}

/// <summary>
/// メタデータを取得
/// </summary>
/// <param name="textureIndex">SRVヒープ内のテクスチャインデックス</param>
/// <returns>指定テクスチャの幅・高さ・フォーマットなどのメタデータ</returns>
const DirectX::TexMetadata &TextureManager::GetMetaData(uint32_t textureIndex) {
  // 範囲外指定違反チェック（テクスチャ番号が正常範囲内である）
  assert(textureIndex < textureDatas_.size());

  // テクスチャデータの参照を取得
  TextureData &textureData = textureDatas_[textureIndex];

  // そのテクスチャのメタデータを返す
  return textureData.metadata;
}
