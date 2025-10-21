#include "Input.h"

#include <cassert>


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
  hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
  assert(SUCCEEDED(hr));

  // 入力データ形式セット
  hr = keyboard_->SetDataFormat(&c_dfDIKeyboard);
  assert(SUCCEEDED(hr));

  // 排他制御レベルのセット
  hr = keyboard_->SetCooperativeLevel(
      hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
  assert(SUCCEEDED(hr));
}
/// <summary>
/// 更新
/// </summary>
void Input::Update() {
  // キーボード情報の取得開始
  keyboard_->Acquire();

  BYTE keys[256] = {};
  BYTE preKeys[256] = {};

  // 全キー入力状態を取得する
  memcpy(preKeys, keys, sizeof(keys));
  keyboard_->GetDeviceState(sizeof(keys), keys);

  if (keys[DIK_SPACE]) {
    OutputDebugStringA("Hit");
  }
  
} 
