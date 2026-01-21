#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <cstdint>
#include <wrl.h>

class WinApp;

class Input {
public:
  //================================================================================
  // シングルトン
  //================================================================================

  // 唯一のインスタンス取得
  static Input *GetInstance();

  static void Shutdown();

  /// <summary>
  /// コンストラクタ
  /// </summary>
  Input() = default;
  /// <summary>
  /// デストラクタ
  /// </summary>
  ~Input() = default;

private:
  static Input *instance;

  /// <summary>
  /// コピーコンストラクタ禁止
  /// </summary>
  /// <param name="">コピー元（使用不可）</param>
  Input(Input &) = delete;
  /// <summary>
  /// 代入演算子禁止
  /// </summary>
  /// <param name="">代入元（使用不可）</param>
  /// <returns>このオブジェクトを返す</returns>
  Input &operator=(Input &) = delete;

public:
  //================================================================================
  // 初期化 / 更新
  //================================================================================

  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(WinApp *winApp);
  /// <summary>
  /// 更新
  /// </summary>
  void Update();

public:
  //================================================================================
  // キーの入力判定
  //================================================================================

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
  WinApp *winApp_ = nullptr;

  //================================================================================
  // DirectInput本体 / デバイス
  //================================================================================

  // DirectInput
  ComPtr<IDirectInput8> directInput_ = nullptr;

  // キーボードデバイス
  ComPtr<IDirectInputDevice8> keyboard_ = nullptr;

  //================================================================================
  // キーボード入力状態
  //================================================================================

  // 現在のキー
  BYTE keys_[256] = {};
  // 前のキー
  BYTE preKeys_[256] = {};
};
