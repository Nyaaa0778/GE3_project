#pragma once
#include <Windows.h>
#include <cstdint>

class WinApp {
public:
  // クライアント領域のサイズ
  static inline const int32_t kClientWidth = 1280;
  static inline const int32_t kClientHeight = 720;

private:
  // ウィンドウクラスの設定
  WNDCLASS wc_{};

  // ウィンドウハンドル
  HWND hwnd_ = nullptr;

public:
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
