#include "WinApp.h"

#ifdef USE_IMGUI

#include <imgui_impl_win32.h>

#endif

#pragma comment(lib, "winmm.lib")

#ifdef USE_IMGUI

//================================================================================
// メッセージハンドラ
//================================================================================

/// <summary>
/// ImGuiのメッセージ処理
/// </summary>
/// <param name="hwnd">ウィンドウハンドル</param>
/// <param name="msg">受信したメッセージID</param>
/// <param name="wParam">メッセージごとの追加情報(上位)</param>
/// <param name="lParam">メッセージごとの追加情報(下位)</param>
/// <returns></returns>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

#endif

//================================================================================
// シングルトン
//================================================================================

std::unique_ptr<WinApp> WinApp::instance = nullptr;

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>WinAppの唯一のインスタンス</returns>
WinApp *WinApp::GetInstance() {
  if (instance == nullptr) {
    instance.reset(new WinApp());
  }

  return instance.get();
}

void WinApp::Finalize() { instance.reset(); }

WinApp::~WinApp() {
  CloseWindow(hwnd_);
  CoUninitialize();
}

//================================================================================
// 初期化 / メッセージ処理 / 終了
//================================================================================

/// <summary>
/// 初期化
/// </summary>
/// <param name="title">ウィンドウタイトル</param>
/// <param name="width">横幅（指定なしなら1280）</param>
/// <param name="height">縦幅（指定なしなら720）</param>
void WinApp::Initialize(const wchar_t *title, int32_t width, int32_t height) {

  // 画面サイズを記録
  width_ = width;
  height_ = height;

  // FPS固定のシステムタイマーの分解能をあげる
  timeBeginPeriod(1);

  // ウィンドウプロシージャ
  wc_.lpfnWndProc = WindowProc;
  // ウィンドウクラス名
  wc_.lpszClassName = L"CG2WindowClass";
  // インスタンスハンドル
  wc_.hInstance = GetModuleHandle(nullptr);
  // カーソル
  wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);

  // ウィンドウクラスを登録する
  RegisterClass(&wc_);

  // ウィンドウサイズを表す構造体にクライアント領域を入れる
  RECT wrc = {0, 0, width_, height_};

  // クライアント領域をもとに実際のサイズにwrcを変更してもらう
  AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

  hwnd_ =
      CreateWindow(wc_.lpszClassName, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                   CW_USEDEFAULT, wrc.right - wrc.left, wrc.bottom - wrc.top,
                   nullptr, nullptr, wc_.hInstance, nullptr);

  ShowWindow(hwnd_, SW_SHOW);
}

/// <summary>
/// メッセージの処理
/// </summary>
/// <returns>アプリ終了メッセージを受け取ったらtrue、それ以外はfalse</returns>
bool WinApp::ProcessMessage() {
  MSG msg{};

  // メッセージがある限りループし続ける
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);

    // ループの中で終了メッセージをチェックする
    if (msg.message == WM_QUIT) {
      return true;
    }
  }

  return false;
}

/// <summary>
/// ウィンドウに送られてくるメッセージを受け取って処理
/// </summary>
/// <param name="hwnd">ウィンドウハンドル</param>
/// <param name="msg">受信したメッセージID</param>
/// <param name="wparam">メッセージごとの追加情報(上位)</param>
/// <param name="lparam">メッセージごとの追加情報(下位)</param>
/// <returns></returns>
LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam,
                                    LPARAM lparam) {
#ifdef USE_IMGUI

  if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
    return true;
  }

#endif

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
