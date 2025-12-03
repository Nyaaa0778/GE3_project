#pragma once

#ifdef USE_IMGUI

#include "../externals/imgui/imgui.h"
#include "../externals/imgui/imgui_impl_dx12.h"
#include "../externals/imgui/imgui_impl_win32.h"

#endif

class WinApp;
class DirectXCommon;
class ShaderResourceViewManager;

class ImGuiManager {
public:
  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="winApp">WinAppのポインタ</param>
  /// <param name="dxCommon">DirectXCommonのポインタ</param>
  /// <param name="srvManager">SrvManagerのポインタ</param>
  void Initialize(WinApp *winApp, DirectXCommon *dxCommon,
                  ShaderResourceViewManager *srvManager);

  /// <summary>
  /// ImGui受付開始
  /// </summary>
  void Begin();
  /// <summary>
  /// ImGui受付終了
  /// </summary>
  void End();

  /// <summary>
  /// 描画
  /// </summary>
  void Draw();

  /// <summary>
  /// 終了
  /// </summary>
  void Finalize();

private:
  //================================================================================
  // 外部参照
  //================================================================================

  // DirectXCommonのポインタ
  DirectXCommon *dxCommon_ = nullptr;
  // SrvManager
  ShaderResourceViewManager *srvManager_ = nullptr;
};
