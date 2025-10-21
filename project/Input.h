#pragma once
#include <Windows.h>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#include <wrl.h>
#include <cstdint>

class Input {
private:
  // キーボードデバイス
  Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_ = nullptr;

  // namespace
  template <class InterfaceType>
  using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(HINSTANCE hInstance, HWND hwnd);
  /// <summary>
  /// 更新
  /// </summary>
  void Update();
};
