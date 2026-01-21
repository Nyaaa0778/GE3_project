#pragma once

#include <d3d12.h>
#include <wrl.h>

class DirectXCommon;

class SpriteRenderer {
public:
  //================================================================================
  // シングルトン
  //================================================================================

  // 唯一のインスタンス取得
  static SpriteRenderer *GetInstance();

  static void Shutdown();

  /// <summary>
  /// コンストラクタ
  /// </summary>
  SpriteRenderer() = default;
  /// <summary>
  /// デストラクタ
  /// </summary>
  ~SpriteRenderer() = default;

private:
  static SpriteRenderer *instance;

  /// <summary>
  /// コピーコンストラクタ禁止
  /// </summary>
  /// <param name="">コピー元（使用不可）</param>
  SpriteRenderer(SpriteRenderer &) = delete;
  /// <summary>
  /// 代入演算子禁止
  /// </summary>
  /// <param name="">代入元（使用不可）</param>
  /// <returns>このオブジェクトを返す</returns>
  SpriteRenderer &operator=(SpriteRenderer &) = delete;

public:
  //================================================================================
  // 初期化 / 描画設定
  //================================================================================

  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="dxCommon">DirectXCommonのポインタ</param>
  void Initialize(DirectXCommon *dxCommon);

  /// <summary>
  /// 共通描画設定
  /// </summary>
  void SetupCommonRenderState();

public:
  //================================================================================
  // Getter
  //================================================================================

  // DirectXCommon
  DirectXCommon *GetDxCommon() const { return dxCommon_; }

private:
  //================================================================================
  // 型エイリアス
  //================================================================================

  // namespace
  template <class InterfaceType>
  using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

  //================================================================================
  // 外部参照
  //================================================================================

  // DirectXCommonのポインタ
  DirectXCommon *dxCommon_ = nullptr;

  //================================================================================
  // GPU リソース
  //================================================================================

  // ルートシグネチャ
  ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
  // グラフィックスパイプラインステート
  ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;

private:
  //================================================================================
  // パイプライン構築（RootSignature / PSO）
  //================================================================================

  /// <summary>
  /// ルートシグネチャの生成
  /// </summary>
  void CreateRootSignature();

  /// <summary>
  /// グラフィックスパイプラインの生成
  /// </summary>
  void CreateGraphicsPipeline();
};
