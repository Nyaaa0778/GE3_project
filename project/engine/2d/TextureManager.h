#pragma once

#include "../../externals/DirectXTex/DirectXTex.h"
#include <d3d12.h>
#include <string>
#include <wrl.h>

class DirectXCommon;

class TextureManager {
public:
  /// <summary>
  /// シングルトンインスタンスの取得
  /// </summary>
  /// <returns>TextureManager の唯一のインスタンス</returns>
  static TextureManager *GetInstance();

  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="dxCommon">DirectXCommonのポインタ</param>
  void Initialize(DirectXCommon *dxCommon);

  /// <summary>
  /// 終了
  /// </summary>
  void Finalize();

  /// <summary>
  /// 画像ファイルを読み込みテクスチャに変換
  /// </summary>
  /// <param name="filePath"></param>
  /// <returns></returns>
  void LoadTexture(const std::string &filePath);

  /// <summary>
  /// 中間リソースの解放
  /// </summary>
  void ReleaseIntermediateResources();

  /// <summary>
  /// SRVインデックスの取得
  /// </summary>
  /// <param name="filePath">検索対象となるテクスチャのファイルパス</param>
  /// <returns>見つかったテクスチャのSRVインデックス、
  /// 該当するテクスチャが存在しない場合はassertによりプログラムを停止する</returns>
  uint32_t GetSrvIndexByFilePath(const std::string &filePath);
  /// <summary>
  /// SRVハンドルの取得
  /// </summary>
  /// <param name="textureIndex">SRVヒープ内のテクスチャインデックス</param>
  /// <returns>指定インデックスのSRVのGPUデスクリプタハンドル</returns>
  D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);

  /// <summary>
  /// メタデータを取得
  /// </summary>
  /// <param name="textureIndex">SRVヒープ内のテクスチャインデックス</param>
  /// <returns>指定テクスチャの幅・高さ・フォーマットなどのメタデータ</returns>
  const DirectX::TexMetadata &GetMetaData(uint32_t textureIndex);

private:
  // namespace
  template <class InterfaceType>
  using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

private:
  // テクスチャ一枚分のデータ
  struct TextureData {
    std::string filePath;
    DirectX::TexMetadata metadata;
    ComPtr<ID3D12Resource> resource;
    ComPtr<ID3D12Resource> intermediateResource;
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
  };

private:
  // DirectXCommonのポインタ
  DirectXCommon *dxCommon_ = nullptr;

  // テクスチャデータ
  std::vector<TextureData> textureDatas_;

  // SRVインデックスの開始番号
  static uint32_t kSRVIndexTop;

private:
  static TextureManager *instance;

  /// <summary>
  /// コンストラクタ
  /// </summary>
  TextureManager() = default;
  /// <summary>
  /// デストラクタ
  /// </summary>
  ~TextureManager() = default;

  /// <summary>
  /// コピーコンストラクタ禁止
  /// </summary>
  /// <param name="">コピー元（使用不可）</param>
  TextureManager(TextureManager &) = delete;
  /// <summary>
  /// 代入演算子禁止
  /// </summary>
  /// <param name="">代入元（使用不可）</param>
  /// <returns>このオブジェクトを返す</returns>
  TextureManager &operator=(TextureManager &) = delete;
};
