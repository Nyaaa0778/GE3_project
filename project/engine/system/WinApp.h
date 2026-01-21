#pragma once
#include <Windows.h>
#include <cstdint>

class WinApp {
public:
  //================================================================================
  // 画面サイズ
  //================================================================================

  // クライアント領域のサイズ
  static const int32_t kClientWidth = 1280;
  static const int32_t kClientHeight = 720;

public:
  //================================================================================
  // シングルトン
  //================================================================================

  // 唯一のインスタンス取得
  static WinApp *GetInstance();

  static void Shutdown();

  /// <summary>
  /// コンストラクタ
  /// </summary>
  WinApp() = default;
  /// <summary>
  /// デストラクタ
  /// </summary>
  ~WinApp() = default;

private:
  static WinApp *instance;

  /// <summary>
  /// コピーコンストラクタ禁止
  /// </summary>
  /// <param name="">コピー元（使用不可）</param>
  WinApp(WinApp &) = delete;
  /// <summary>
  /// 代入演算子禁止
  /// </summary>
  /// <param name="">代入元（使用不可）</param>
  /// <returns>このオブジェクトを返す</returns>
  WinApp &operator=(WinApp &) = delete;

public:
  //================================================================================
  // 初期化 / メッセージ処理 / 終了
  //================================================================================

  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize();

  /// <summary>
  /// メッセージの処理
  /// </summary>
  /// <returns></returns>
  bool ProcessMessage();

  /// <summary>
  /// 終了
  /// </summary>
  void Finalize();

  //================================================================================
  // Getter
  //================================================================================

  /// <summary>
  /// アプリケーションハンドルを取得
  /// </summary>
  /// <returns>アプリケーションハンドル</returns>
  HINSTANCE GetHInstance() const { return wc_.hInstance; }
  /// <summary>
  /// ウィンドウハンドルを取得
  /// </summary>
  /// <returns>ウィンドウハンドル</returns>
  HWND GetHwnd() const { return hwnd_; }

private:
  //================================================================================
  // ウィンドウ情報
  //================================================================================

  // ウィンドウクラスの設定
  WNDCLASS wc_{};

  // ウィンドウハンドル
  HWND hwnd_ = nullptr;

private:
  //================================================================================
  // メッセージハンドラ
  //================================================================================

  /// <summary>
  /// ウィンドウに送られてくるメッセージを受け取って処理
  /// </summary>
  /// <param name="hwnd">ウィンドウハンドル</param>
  /// <param name="msg">受信したメッセージID</param>
  /// <param name="wparam">メッセージごとの追加情報(上位)</param>
  /// <param name="lparam">メッセージごとの追加情報(下位)</param>
  /// <returns></returns>
  static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam,
                                     LPARAM lparam);
};
