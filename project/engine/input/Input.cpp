#include "Input.h"
#include "WinApp.h"

#include <cassert>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

//================================================================================
// シングルトン
//================================================================================

Input *Input::instance = nullptr;

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>Inputの唯一のインスタンス</returns>
Input *Input::GetInstance() {
  if (instance == nullptr) {
    instance = new Input;
  }

  return instance;
}

void Input::Shutdown() {
  delete instance;
  instance = nullptr;
}

//================================================================================
// 初期化 / 更新
//================================================================================

/// <summary>
/// 初期化
/// </summary>
void Input::Initialize(WinApp *winApp) {
  // 引数で受け取ってメンバ変数に保存
  winApp_ = winApp;

  HRESULT hr;

  // DirectInputの初期化
  hr = DirectInput8Create(winApp_->GetHInstance(), DIRECTINPUT_VERSION,
                          IID_IDirectInput8, (void **)&directInput_, nullptr);
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
}

//================================================================================
// キーの入力判定
//================================================================================

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
