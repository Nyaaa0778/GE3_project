#pragma once

#include <d3d12.h>
#include <wrl.h>

class DirectXCommon;
class Camera;

class Object3dRenderer {
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

  //================================================================================
  // Getter
  //================================================================================

  // DirectXCommon
  DirectXCommon *GetDxCommon() const { return dxCommon_; }

  // デフォルトカメラ
  Camera *GetDefaultCamera() const { return defaultCamera_; }

  //================================================================================
  // Setter
  //================================================================================

  // デフォルトカメラ
  void SetDefaultCamera(Camera *defaultCamera) {
    defaultCamera_ = defaultCamera;
  }

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

  // デフォルトカメラ
  Camera *defaultCamera_ = nullptr;

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
  /// ルートシグネチャを作成
  /// </summary>
  void CreateRootSignature();
  /// <summary>
  /// グラフィックスパイプラインの生成
  /// </summary>
  void CreateGraphicsPipeline();
};
