#pragma once
#include <wrl.h>
#include <d3d12.h>

class DirectXCommon;

class SpriteCommon {
public:
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
  DirectXCommon *GetDxCommon() const { return dxCommon_; }

private:
  /// <summary>
  /// ルートシグネチャの生成
  /// </summary>
  void CreateRootSignature();

  /// <summary>
  /// グラフィックスパイプラインの生成
  /// </summary>
  void CreateGraphicsPipeline();

private:
  // namespace
  template <class InterfaceType>
  using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

  // DirectXCommonのポインタ
  DirectXCommon *dxCommon_ = nullptr;

  ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;

  ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;
};
