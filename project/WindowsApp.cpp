#include "WindowsApp.h"
#include <Windows.h>
#include <cstdint>

/// <summary>
/// 初期化
/// </summary>
void WindowsApp::Initialize() {
  WNDCLASS wc{};
  // ウィンドウプロシージャ
  wc.lpfnWndProc = WindowProc;
  // ウィンドウクラス名
  wc.lpszClassName = L"CG2WindowClass";
  // インスタンスハンドル
  wc.hInstance = GetModuleHandle(nullptr);
  // カーソル
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

  // ウィンドウクラスを登録する
  RegisterClass(&wc);

  // クライアント領域のサイズ
  const int32_t kClientWidth = 1280;
  const int32_t kClientHeight = 720;

  // ウィンドウサイズを表す構造体にクライアント領域を入れる
  RECT wrc = {0, 0, kClientWidth, kClientHeight};

  // クライアント領域をもとに実際のサイズにwrcを変更してもらう
  AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

  HWND hwnd =
      CreateWindow(wc.lpszClassName, L"CG2", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                   CW_USEDEFAULT, wrc.right - wrc.left, wrc.bottom - wrc.top,
                   nullptr, nullptr, wc.hInstance, nullptr);

#ifdef _DEBUG
  ComPtr<ID3D12Debug1> debugController = nullptr;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
    // デバッグレイヤーを有効化
    debugController->EnableDebugLayer();
    // さらにGPU側でもチェック
    debugController->SetEnableGPUBasedValidation(TRUE);
  }
#endif

  ShowWindow(hwnd, SW_SHOW);
}
/// <summary>
/// 更新
/// </summary>
void WindowsApp::Update() {}

/// <summary>
/// 
/// </summary>
/// <param name="hwnd">ウィンドウハンドル</param>
/// <param name="msg">受信したメッセージID</param>
/// <param name="wparam">メッセージごとの追加情報(上位)</param>
/// <param name="lparam">メッセージごとの追加情報(下位)</param>
/// <returns></returns>
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  /*if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
    return true;
  }*/

  switch (msg) {
    // ウィンドウが破壊された
  case WM_DESTROY:
    // QSに対して、アプリの終了を伝える
    PostQuitMessage(0);
    return 0;
  }

  // 標準のメッセージ処理を行う
  return DefWindowProc(hwnd, msg, wparam, lparam);
}
