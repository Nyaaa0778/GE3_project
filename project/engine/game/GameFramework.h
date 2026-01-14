#pragma once

#include "D3DResourceLeakChecker.h"

class Camera;
class DirectXCommon;
class Input;
class Object3dRenderer;
class ShaderResourceViewManager;
class SoundManager;
class SpriteRenderer;
class WinApp;
class ImGuiManager;

class GameFramework {
public:
  virtual ~GameFramework() = default;

  // 実行
  void Execute();

  // 初期化
  virtual void Initialize();
  // 更新
  virtual void Update();
  // 描画
  virtual void Draw() = 0;
  // 終了
  virtual void Finalize();

  // 終了チェック
  virtual bool IsEndRequest() { return endRequest_; }

private:
  // WinAppのポインタ
  WinApp *winApp_ = nullptr;
  // DirectXCommonのポインタ
  DirectXCommon *dxCommon_ = nullptr;
  // srvManagerのポインタ
  ShaderResourceViewManager *srvManager_ = nullptr;
  // SpriteRendererのポインタ
  SpriteRenderer *spriteRenderer_ = nullptr;
  // Object3dRendererのポインタ
  Object3dRenderer *object3dRenderer_ = nullptr;
  // Inputのポインタ
  Input *input_ = nullptr;
  // SoundManagerのポインタ
  SoundManager *soundManager_ = nullptr;
  // Cameraのポインタ
  Camera *camera_ = nullptr;

  // リークチェック
  D3DResourceLeakChecker leakCheck_;

  // ImGuiManagerのポインタ
#ifdef USE_IMGUI
  ImGuiManager *imguiManager_ = nullptr;
#endif

  // ゲーム終了フラグ
  bool endRequest_ = false;
};
