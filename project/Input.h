#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#include <cstdint>
#include <wrl.h>

#include "WinApp.h"

class Input {
private:
  //WinApp
  WinApp *winApp_ = nullptr;

  // namespace
  template <class InterfaceType>
  using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

  // DirectInput
  ComPtr<IDirectInput8> directInput_ = nullptr;

  // キーボードデバイス
  ComPtr<IDirectInputDevice8> keyboard_ = nullptr;

  // 現在のキー
  BYTE keys_[256] = {};
  // 前のキー
  BYTE preKeys_[256] = {};

public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(WinApp* winApp);
  /// <summary>
  /// 更新
  /// </summary>
  void Update();

  /// <summary>
  /// Push処理
  /// </summary>
  /// <param name="keyCode">押下状態を確認するキーコード</param>
  /// <returns>押下状態ならtrue、それ以外ならfalse</returns>
  bool PushKey(BYTE keyCode);
  /// <summary>
  /// Trigger処理
  /// </summary>
  /// <param name="keyCode">押下状態を確認するキーコード</param>
  /// <returns>押下状態ならtrue、それ以外ならfalse</returns>
  bool TriggerKey(BYTE keyCode);
};
