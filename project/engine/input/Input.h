#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <Xinput.h>

#include <cstdint>
#include <memory>
#include <wrl.h>

class WinApp;

class Input {
public:
	//================================================================================
	// シングルトン
	//================================================================================

	// 唯一のインスタンス取得
	static Input* GetInstance();

	/// <summary>
	/// コンストラクタ
	/// </summary>
	Input() = default;
	/// <summary>
	/// デストラクタ
	/// </summary>
	~Input() = default;


	/// <summary>
	/// 終了
	/// </summary>
	static void Finalize();

	// unique_ptrからの削除を許可
	friend std::default_delete<Input>;

private:
	static std::unique_ptr<Input> instance;

	/// <summary>
	/// コピーコンストラクタ禁止
	/// </summary>
	/// <param name="">コピー元（使用不可）</param>
	Input(Input&) = delete;
	/// <summary>
	/// 代入演算子禁止
	/// </summary>
	/// <param name="">代入元（使用不可）</param>
	/// <returns>このオブジェクトを返す</returns>
	Input& operator=(Input&) = delete;

public:
	//================================================================================
	// 初期化 / 更新
	//================================================================================

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(WinApp* winApp);
	/// <summary>
	/// 更新
	/// </summary>
	void Update();

public:
	//================================================================================
	// キーの入力判定
	//================================================================================

	/// <summary>
	/// キーボードのPush処理
	/// </summary>
	/// <param name="keyCode">押下状態を確認するキーコード</param>
	bool PushKey(BYTE keyCode);
	/// <summary>
	/// キーボードのTrigger処理
	/// </summary>
	/// <param name="keyCode">押下状態を確認するキーコード</param>
	bool TriggerKey(BYTE keyCode);

	/// <summary>
	/// キーボードのRelease処理
	/// </summary>
	/// <param name="keyCode">押下状態を確認するキーコード</param>
	bool ReleaseKey(BYTE keyCode);

public:
	//================================================================================
	// マウスの入力判定
	//================================================================================

	/// <summary>
	/// マウスボタンのPush処理
	/// </summary>
	/// <param name="buttonNumber">0:左, 1:右, 2:中, 3~:その他</param>
	bool PushMouse(int buttonNumber);
	/// <summary>
	/// マウスボタンのTrigger処理
	/// </summary>
	/// <param name="buttonNumber">0:左, 1:右, 2:中, 3~:その他</param>
	bool TriggerMouse(int buttonNumber);
	/// <summary>
	/// マウスボタンのRelease処理
	/// </summary>
	/// <param name="buttonNumber">0:左, 1:右, 2:中, 3~:その他</param>
	bool ReleaseMouse(int buttonNumber);

	// 移動量データ
	struct MouseMove {
		int lX; // 横移動量
		int lY; // 縦移動量
		int lZ; // ホイール
	};

	// 座標データ
	struct MousePos {
		int x; // 左上からのX座標
		int y; // 左上からのY座標
	};

	/// <summary>
	/// マウス移動量の取得
	/// </summary>
	MouseMove GetMouseMove();

	/// <summary>
	/// マウス座標の取得
	/// </summary>
	MousePos GetMousePosition();

	/// <summary>
	/// マウスカーソルの表示設定
	/// </summary>
	/// <param name="visible">表示するかどうか</param>
	void SetCursorVisible(bool visible);

public:
	//================================================================================
	// ゲームパッドの入力判定
	//================================================================================

	/// <summary>
	/// ゲームパッドのPush処理
	/// </summary>
	/// <param name="button">ボタンの種類</param>
	bool PushButton(WORD button);
	/// <summary>
	/// ゲームパッドのTrigger処理
	/// </summary>
	/// <param name="button">ボタンの種類</param>
	bool TriggerButton(WORD button);
	/// <summary>
	/// ゲームパッドのRelease処理
	/// </summary>
	/// <param name="button">ボタンの種類</param>
	bool ReleaseButton(WORD button);

	// スティックの入力データ
	struct Stick {
		float x;
		float y;
	};

	/// <summary>
	/// 左スティックの入力を取得
	/// </summary>
	/// <param name="deadZone">デッドゾーン（遊び）のしきい値</param>
	Stick GetLeftStick(float deadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	/// <summary>
	/// 右スティックの入力を取得
	/// </summary>
	/// <param name="deadZone">デッドゾーン（遊び）のしきい値</param>
	Stick GetRightStick(float deadZone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

	/// <summary>
	/// 左トリガーの押し込み量を取得
	/// </summary>
	/// <returns>0.0f(離) ～ 1.0f(全押)</returns>
	float GetLeftTrigger();
	/// <summary>
	/// 右トリガーの押し込み量を取得
	/// </summary>
	/// <returns>0.0f(離) ～ 1.0f(全押)</returns>
	float GetRightTrigger();

	/// <summary>
	/// ゲームパッドの振動設定
	/// </summary>
	/// <param name="leftMotor">左モーター（低周波）0.0f ～ 1.0f</param>
	/// <param name="rightMotor">右モーター（高周波）0.0f ～ 1.0f</param>
	void SetShake(float leftMotor, float rightMotor);
	/// <summary>
	/// ゲームパッドの振動設定
	/// </summary>
	/// <param name="leftMotor">左モーター（低周波）0.0f ～ 1.0f</param>
	/// <param name="rightMotor">右モーター（高周波）0.0f ～ 1.0f</param>
	/// <param name="frames">振動時間</param>
	void SetShake(float leftMotor, float rightMotor, float frames);

private:
	//================================================================================
	// 型エイリアス
	//================================================================================

	// namespace
	template <class InterfaceType>
	using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

private:
	//================================================================================
	// 外部参照
	//================================================================================

	// WinAppのポインタ
	WinApp* winApp_ = nullptr;

private:
	//================================================================================
	// DirectInput本体
	//================================================================================

	// DirectInput
	ComPtr<IDirectInput8> directInput_ = nullptr;

private:
	//================================================================================
	// キーボードデバイス
	//================================================================================

	// キーボードデバイス
	ComPtr<IDirectInputDevice8> keyboard_ = nullptr;

	// 現在のキー
	BYTE keys_[256] = {};
	// 前のキー
	BYTE preKeys_[256] = {};

private:
	//================================================================================
	// マウスデバイス
	//================================================================================

	// マウスデバイス
	ComPtr<IDirectInputDevice8> mouse_ = nullptr;

	// 現在のマウス状態
	DIMOUSESTATE mouseState_ = {};
	// 前のマウス状態
	DIMOUSESTATE preMouseState_ = {};

	// カーソルの表示状態を覚えておく変数
	bool isShowCursor_ = true;

private:
	//================================================================================
	// ゲームパッドデバイス
	//================================================================================

	// 現在のゲームパッドの状態
	XINPUT_STATE xinputState_ = {};
	// 前のゲームパッドの状態
	XINPUT_STATE preXinputState_ = {};

	// 接続されているか
	bool isConnected_ = false;

	// 振動時間
	float shakeTimer_ = 0;
};
