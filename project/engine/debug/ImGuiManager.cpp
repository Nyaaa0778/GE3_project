#include "ImGuiManager.h"
#include "DirectXCommon.h"
#include "ShaderResourceViewManager.h"
#include "WinApp.h"

//================================================================================
// シングルトン
//================================================================================

ImGuiManager *ImGuiManager::instance = nullptr;

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>ImGuiManagerの唯一のインスタンス</returns>
ImGuiManager *ImGuiManager::GetInstance() {
  if (instance == nullptr) {
    instance = new ImGuiManager;
  }

  return instance;
}

void ImGuiManager::Shutdown() {
  delete instance;
  instance = nullptr;
}

/// <summary>
/// 初期化
/// </summary>
/// <param name="winApp">WinAppのポインタ</param>
/// <param name="dxCommon">DirectXCommonのポインタ</param>
/// <param name="srvManager">SrvManagerのポインタ</param>
void ImGuiManager::Initialize(
    [[maybe_unused]] WinApp *winApp, [[maybe_unused]] DirectXCommon *dxCommon,
    [[maybe_unused]] ShaderResourceViewManager *srvManager) {
#ifdef USE_IMGUI

  dxCommon_ = dxCommon;
  srvManager_ = srvManager;

  uint32_t fontSrvIndex = srvManager_->Allocate();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  {
    ImGuiIO &io = ImGui::GetIO();
    // デフォルトフォントを登録
    if (io.Fonts->Fonts.empty()) {
      io.Fonts->AddFontDefault();
    }
    // フォントアトラスを明示的にビルド
    io.Fonts->Build();
  }

  ImGui::StyleColorsClassic();
  ImGui_ImplWin32_Init(winApp->GetHwnd());
  ImGui_ImplDX12_Init(dxCommon_->GetDevice(),
                      static_cast<int>(dxCommon_->GetSwapChainResourceNum()),
                      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                      srvManager_->GetDescriptorHeap(),
                      srvManager_->GetCPUDescriptorHandle(fontSrvIndex),
                      srvManager_->GetGPUDescriptorHandle(fontSrvIndex));

#endif
}

/// <summary>
/// ImGui受付開始
/// </summary>
void ImGuiManager::Begin() {
#ifdef USE_IMGUI

  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

#endif
}
/// <summary>
/// ImGui受付終了
/// </summary>
void ImGuiManager::End() {
#ifdef USE_IMGUI

  ImGui::Render();

#endif
}

/// <summary>
/// 描画
/// </summary>
void ImGuiManager::Draw() {
#ifdef USE_IMGUI

  ID3D12GraphicsCommandList *commandList = dxCommon_->GetCommandList();

  // デスクリプタヒープの配列をセットするコマンド
  ID3D12DescriptorHeap *ppHeaps[] = {srvManager_->GetDescriptorHeap()};
  commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
  // 描画コマンド発行
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

#endif
}

/// <summary>
/// 終了
/// </summary>
void ImGuiManager::Finalize() {
#ifdef USE_IMGUI

  // ImGuiの終了処理、初期化と逆順に行う
  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

#endif
}
