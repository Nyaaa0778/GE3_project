#include "Input.h"
#include "TimeManager.h"
#include "WinApp.h"

#include <cassert>
#include <cmath>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#pragma comment(lib, "xinput.lib")

//================================================================================
// シングルトン
//================================================================================

std::unique_ptr<Input> Input::instance = nullptr;

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>Inputの唯一のインスタンス</returns>
Input* Input::GetInstance() {
	if (instance == nullptr) {
		instance = std::make_unique<Input>();
	}

	return instance.get();
}

/// <summary>
/// 終了
/// </summary>
void Input::Finalize() { instance.reset(); }

//================================================================================
// 初期化 / 更新
//================================================================================

/// <summary>
/// 初期化
/// </summary>
void Input::Initialize(WinApp* winApp) {
	// 引数で受け取ってメンバ変数に保存
	winApp_ = winApp;

	HRESULT hr;

	// DirectInputの初期化
	hr = DirectInput8Create(winApp_->GetHInstance(), DIRECTINPUT_VERSION,
		IID_IDirectInput8, (void**) &directInput_, nullptr);
	assert(SUCCEEDED(hr));

	/*---------- キーボード ----------*/

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

	/*---------- マウス ----------*/

	hr = directInput_->CreateDevice(GUID_SysMouse, &mouse_, NULL);
	assert(SUCCEEDED(hr));

	// 入力データ形式セット（マウス用）
	hr = mouse_->SetDataFormat(&c_dfDIMouse);
	assert(SUCCEEDED(hr));

	// 排他制御レベルのセット
	// マウスは画面外に出ないように排他(EXCLUSIVE)にする場合もありますが、
	// デバッグしにくくなるため通常はNONEXCLUSIVEでOKです。
	hr = mouse_->SetCooperativeLevel(winApp_->GetHwnd(), DISCL_FOREGROUND |
		DISCL_NONEXCLUSIVE |
		DISCL_NOWINKEY);
	assert(SUCCEEDED(hr));

	/*---------- ゲームパッド ----------*/

	// ゲームパッドの状態を保存
	preXinputState_ = xinputState_;

	// ゲームパッド情報の取得
	DWORD dwResult = XInputGetState(0, &xinputState_);
	isConnected_ = (dwResult == ERROR_SUCCESS);
}
/// <summary>
/// 更新
/// </summary>
void Input::Update() {

	/*---------- キーボード ----------*/

	// キーボード情報の取得開始
	keyboard_->Acquire();
	// 全キー入力状態を取得する
	memcpy(preKeys_, keys_, sizeof(keys_));
	keyboard_->GetDeviceState(sizeof(keys_), keys_);

	/*---------- マウス ----------*/

	// マウス情報の取得開始
	mouse_->Acquire();
	// 前回の状態を保存
	preMouseState_ = mouseState_;
	// 現在の状態を取得
	mouse_->GetDeviceState(sizeof(DIMOUSESTATE), &mouseState_);

	/*---------- ゲームパッド ----------*/

	// ゲームパッドの状態を保存
	preXinputState_ = xinputState_;
	// ゲームパッド情報の取得
	DWORD dwResult = XInputGetState(0, &xinputState_);

	isConnected_ = (dwResult == ERROR_SUCCESS);

	float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

	if (shakeTimer_ > 0.0f) {
		shakeTimer_ -= deltaTime;

		if (shakeTimer_ == 0.0f) {
			// 時間が切れたら物理的に振動を止める
			XINPUT_VIBRATION vibration = {0, 0};
			XInputSetState(0, &vibration);
		}
	}
}

//================================================================================
// キーの入力判定
//================================================================================

/// <summary>
/// Push処理
/// </summary>
/// <param name="keyCode">押下状態を確認するキーコード</param>
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
bool Input::TriggerKey(BYTE keyCode) {
	// 指定したキーを押していればtrue
	if (keys_[keyCode] && !preKeys_[keyCode]) {
		return true;
	}

	// その他のキーを押していればfalse
	return false;
}

/// <summary>
/// キーボードのRelease処理
/// </summary>
/// <param name="keyCode">押下状態を確認するキーコード</param>
bool Input::ReleaseKey(BYTE keyCode) {
	// 前回押していて、今回は押していない
	if (!keys_[keyCode] && preKeys_[keyCode]) {
		return true;
	}

	// その他のキーを押していればfalse
	return false;
}

//================================================================================
// マウスの入力判定
//================================================================================

/// <summary>
/// マウスボタンのPush処理
/// </summary>
/// <param name="buttonNumber">0:左, 1:右, 2:中, 3~:その他</param>
bool Input::PushMouse(int buttonNumber) {
	if ((mouseState_.rgbButtons[buttonNumber] & 0x80)) {
		return true;
	}

	return false;
}
/// <summary>
/// マウスボタンのTrigger処理
/// </summary>
/// <param name="buttonNumber">0:左, 1:右, 2:中, 3~:その他</param>
bool Input::TriggerMouse(int buttonNumber) {
	// 今回押されていて、かつ前回押されていない
	bool current = (mouseState_.rgbButtons[buttonNumber] & 0x80);
	bool previous = (preMouseState_.rgbButtons[buttonNumber] & 0x80);

	if (current && !previous) {
		return true;
	}

	return false;
}
/// <summary>
/// マウスボタンのRelease処理
/// </summary>
/// <param name="buttonNumber">0:左, 1:右, 2:中, 3~:その他</param>
bool Input::ReleaseMouse(int buttonNumber) {
	// 前回押していて、今回は押していない
	bool current = (mouseState_.rgbButtons[buttonNumber] & 0x80);
	bool previous = (preMouseState_.rgbButtons[buttonNumber] & 0x80);

	if (!current && previous) {
		return true;
	}

	return false;
}

/// <summary>
/// マウス移動量の取得
/// </summary>
/// <returns>マウスの移動量</returns>
Input::MouseMove Input::GetMouseMove() {
	Input::MouseMove move;
	move.lX = static_cast<int>(mouseState_.lX);
	move.lY = static_cast<int>(mouseState_.lY);
	move.lZ = static_cast<int>(mouseState_.lZ);
	return move;
}

/// <summary>
/// マウス座標の取得
/// </summary>
/// <returns>マウスの座標</returns>
Input::MousePos Input::GetMousePosition() {
	Input::MousePos pos;
	POINT point;

	// 画面全体の座標を取得
	GetCursorPos(&point);

	// クライアントエリア（ゲーム画面内）の座標に変換
	ScreenToClient(winApp_->GetHwnd(), &point);

	pos.x = static_cast<int>(point.x);
	pos.y = static_cast<int>(point.y);
	return pos;
}

/// <summary>
/// マウスカーソルの表示設定
/// </summary>
/// <param name="visible">表示するかどうか</param>
void Input::SetCursorVisible(bool visible) {
	// すでに指定された状態なら何もしない
	if (isShowCursor_ == visible) {
		return;
	}

	// 状態が変わる時だけ Windows API を呼ぶ
	ShowCursor(visible ? TRUE : FALSE);

	// 状態を記録する
	isShowCursor_ = visible;
}

//================================================================================
// ゲームパッドの入力判定
//================================================================================

/// <summary>
/// ゲームパッドのPush処理
/// </summary>
/// <param name="button">ボタンの種類</param>
bool Input::PushButton(WORD button) {
	if (!isConnected_) {
		return false;
	}

	return (xinputState_.Gamepad.wButtons & button);
}
/// <summary>
/// ゲームパッドのTrigger処理
/// </summary>
/// <param name="button">ボタンの種類</param>
bool Input::TriggerButton(WORD button) {
	if (!isConnected_) {
		return false;
	}

	return (xinputState_.Gamepad.wButtons & button) &&
		!(preXinputState_.Gamepad.wButtons & button);
}
/// <summary>
/// ゲームパッドのRelease処理
/// </summary>
/// <param name="button">ボタンの種類</param>
bool Input::ReleaseButton(WORD button) {
	if (!isConnected_) {
		return false;
	}

	return !(xinputState_.Gamepad.wButtons & button) &&
		(preXinputState_.Gamepad.wButtons & button);
}

/// <summary>
/// 左スティックの入力を取得
/// </summary>
/// <param name="deadZone">デッドゾーン（遊び）のしきい値</param>
Input::Stick Input::GetLeftStick(float deadZone) {
	if (!isConnected_) {
		return {0.0f, 0.0f};
	}

	float x = (float) xinputState_.Gamepad.sThumbLX;
	float y = (float) xinputState_.Gamepad.sThumbLY;
	float magnitude = sqrt(x * x + y * y);

	if (magnitude < deadZone) {
		return {0.0f, 0.0f};
	}

	// -1.0 ~ 1.0 に正規化（最大値 32767 で割る）
	return {x / 32767.0f, y / 32767.0f};
}
/// <summary>
/// 右スティックの入力を取得
/// </summary>
/// <param name="deadZone">デッドゾーン（遊び）のしきい値</param>
Input::Stick Input::GetRightStick(float deadZone) {
	if (!isConnected_) {
		return {0.0f, 0.0f};
	}

	// 右スティックの入力を取得 (RX, RY)
	float x = (float) xinputState_.Gamepad.sThumbRX;
	float y = (float) xinputState_.Gamepad.sThumbRY;
	float magnitude = sqrt(x * x + y * y);

	// デッドゾーン以下なら入力を無視して 0 を返す
	if (magnitude < deadZone) {
		return {0.0f, 0.0f};
	}

	// -1.0 ~ 1.0 に正規化
	return {x / 32767.0f, y / 32767.0f};
}

/// <summary>
/// 左トリガーの押し込み量を取得
/// </summary>
/// <returns>0.0f(離) ～ 1.0f(全押)</returns>
float Input::GetLeftTrigger() {
	if (!isConnected_) {
		return 0.0f;
	}

	// 0~255の値を 0.0~1.0 に正規化
	return (float) xinputState_.Gamepad.bLeftTrigger / 255.0f;
}
/// <summary>
/// 右トリガーの押し込み量を取得
/// </summary>
/// <returns>0.0f(離) ～ 1.0f(全押)</returns>
float Input::GetRightTrigger() {
	if (!isConnected_) {
		return 0.0f;
	}

	return (float) xinputState_.Gamepad.bRightTrigger / 255.0f;
}

/// <summary>
/// ゲームパッドの振動設定
/// </summary>
/// <param name="leftMotor">左モーター（低周波）0.0f ～ 1.0f</param>
/// <param name="rightMotor">右モーター（高周波）0.0f ～ 1.0f</param>
void Input::SetShake(float leftMotor, float rightMotor) {
	if (!isConnected_) {
		return;
	}

	if (shakeTimer_ > 0 && leftMotor == 0.0f && rightMotor == 0.0f) {
		return;
	}

	XINPUT_VIBRATION vibration;
	vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535);
	vibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535);
	XInputSetState(0, &vibration);
}
/// <summary>
/// ゲームパッドの振動設定
/// </summary>
/// <param name="leftMotor">左モーター（低周波）0.0f ～ 1.0f</param>
/// <param name="rightMotor">右モーター（高周波）0.0f ～ 1.0f</param>
/// <param name="frames">振動時間</param>
void Input::SetShake(float left, float right, float frames) {
	shakeTimer_ = frames;
	SetShake(left, right);
}