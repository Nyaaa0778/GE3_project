#pragma once

#include "../../externals/DirectXTex/DirectXTex.h"
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <wrl.h>

class DirectXCommon;
class ShaderResourceViewManager;

class TextureManager {
public:
  //================================================================================
  // シングルトン管理 / 初期化・終了
  //================================================================================

  /// <summary>
  /// シングルトンインスタンスの取得
  /// </summary>
  /// <returns>TextureManagerの唯一のインスタンス</returns>
  static TextureManager *GetInstance();

  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="dxCommon">DirectXCommonのポインタ</param>
  /// <param name="srvManager">SrvManagerのポインタ</param>
  void Initialize(DirectXCommon *dxCommon,
                  ShaderResourceViewManager *srvManager);

  /// <summary>
  /// 終了
  /// </summary>
  void Finalize();

public:
  //================================================================================
  // テクスチャ読み込み / 中間リソース解放
  //================================================================================

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
  uint32_t GetTextureIndexByFilePath(const std::string &filePath);
  /// <summary>
  /// SRVハンドルの取得
  /// </summary>
  /// <param name="filePath">テクスチャのファイルパス</param>
  /// <returns>指定インデックスのSRVのGPUデスクリプタハンドル</returns>
  D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandlGPU(const std::string &filePath);

  /// <summary>
  /// メタデータを取得
  /// </summary>
  /// <param name="filePath">テクスチャのファイルパス</param>
  /// <returns>指定テクスチャの幅・高さ・フォーマットなどのメタデータ</returns>
  const DirectX::TexMetadata &GetMetaData(const std::string& filePath);

private:
  //================================================================================
  // 型エイリアス
  //================================================================================

  // namespace
  template <class InterfaceType>
  using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

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

private:
  //================================================================================
  // 外部参照
  //================================================================================

  // DirectXCommonのポインタ
  DirectXCommon *dxCommon_ = nullptr;

  // SrvManagerのポインタ
  ShaderResourceViewManager *srvManager_ = nullptr;

  //================================================================================
  // テクスチャ管理データ（読み込み済み）
  //================================================================================

  // テクスチャデータ
  std::unordered_map<std::string, TextureData> textureDatas_;

  // SRVインデックスの開始番号
  static uint32_t kSRVIndexTop;

private:
  //================================================================================
  // シングルトン実装詳細
  //================================================================================

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
