#include "Input.h"

#include <cassert>

/// <summary>
/// 初期化
/// </summary>
void Input::Initialize(WinApp *winApp) {
  // メンバ変数に記録
  winApp_ = winApp;

  HRESULT hr;

  // DirectInputの初期化
  hr = DirectInput8Create(winApp_->GetHInstance(), DIRECTINPUT_VERSION,
                          IID_IDirectInput8,
                          (void **)&directInput_, nullptr);
  assert(SUCCEEDED(hr));

  // キーボードデバイスの生成
  hr = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
  assert(SUCCEEDED(hr));

  // 入力データ形式セット
  hr = keyboard_->SetDataFormat(&c_dfDIKeyboard);
  assert(SUCCEEDED(hr));

  // 排他制御レベルのセット
  hr = keyboard_->SetCooperativeLevel(winApp_->GetHwnd(),
                                      DISCL_FOREGROUND | DISCL_NONEXCLUSIVE |
                                          DISCL_NOWINKEY);
  assert(SUCCEEDED(hr));
}
/// <summary>
/// 更新
/// </summary>
void Input::Update() {
  // キーボード情報の取得開始
  keyboard_->Acquire();

  // 全キー入力状態を取得する
  memcpy(preKeys_, keys_, sizeof(keys_));
  keyboard_->GetDeviceState(sizeof(keys_), keys_);

  if (TriggerKey(DIK_0)) {
    OutputDebugStringA("Hit");
  }
}

/// <summary>
/// Push処理
/// </summary>
/// <param name="keyCode">押下状態を確認するキーコード</param>
/// <returns>押下状態ならtrue、それ以外ならfalse</returns>
bool Input::PushKey(BYTE keyCode) {
  // 指定キーを押していればtrue
  if (keys_[keyCode]) {
    return true;
  }

  // その他のキーを押していればfalse
  return false;
}
/// <summary>
/// Trigger処理
/// </summary>
/// <param name="keyCode">押下状態を確認するキーコード</param>
/// <returns>押下状態ならtrue、それ以外ならfalse</returns>
bool Input::TriggerKey(BYTE keyCode) {
  // 指定したキーを押していればtrue
  if (keys_[keyCode] && !preKeys_[keyCode]) {
    return true;
  }

  // その他のキーを押していればfalse
  return false;
}
