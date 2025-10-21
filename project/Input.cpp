#include "Input.h"

#include <dinput.h>
#include <wrl.h>
#include <cassert>

using namespace Microsoft::WRL;

/// <summary>
/// 初期化
/// </summary>
void Input::Initialize(HINSTANCE hInstance, HWND hwnd) {

  HRESULT hr;

  // DirectInputの初期化
  ComPtr<IDirectInput8> directInput = nullptr;
  hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
                           (void **)&directInput, nullptr);

  assert(SUCCEEDED(hr));

  // キーボードデバイスの生成
  ComPtr < IDirectInputDevice8> keyboard = nullptr;
  hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
  assert(SUCCEEDED(hr));

  // 入力データ形式セット
  hr = keyboard->SetDataFormat(&c_dfDIKeyboard);
  assert(SUCCEEDED(hr));

  // 排他制御レベルのセット
  hr = keyboard->SetCooperativeLevel(
      hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
  assert(SUCCEEDED(hr));
}
/// <summary>
/// 更新
/// </summary>
void Input::Update() {}
