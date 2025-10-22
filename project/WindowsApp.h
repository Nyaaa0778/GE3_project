#pragma once
#include <Windows.h>
#include <cstdint>

class WindowsApp {
public:
  // クライアント領域のサイズ
  static inline const int32_t kClientWidth = 1280;
  static inline const int32_t kClientHeight = 720;

public:
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

public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize();
  /// <summary>
  /// 更新
  /// </summary>
  void Update();
};
