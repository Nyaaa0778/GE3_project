#pragma once
#include "DirectXCommon.h"

#include <d3d12.h>
#include <wrl.h>

class SpriteRenderer {
public:
  //========================================
  // 初期化 / 描画設定
  //========================================

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
  //========================================
  // Getter
  //========================================

  // DirectXCommon
  DirectXCommon *GetDxCommon() const { return dxCommon_; }

private:
  //========================================
  // パイプライン構築（RootSignature / PSO）
  //========================================

  /// <summary>
  /// ルートシグネチャの生成
  /// </summary>
  void CreateRootSignature();

  /// <summary>
  /// グラフィックスパイプラインの生成
  /// </summary>
  void CreateGraphicsPipeline();

private:
  //========================================
  // 型エイリアス
  //========================================

  // namespace
  template <class InterfaceType>
  using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

  //========================================
  // 外部参照
  //========================================

  // DirectXCommonのポインタ
  DirectXCommon *dxCommon_ = nullptr;

  //========================================
  // GPU リソース
  //========================================

  // ルートシグネチャ
  ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
  // グラフィックスパイプラインステート
  ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;
};
