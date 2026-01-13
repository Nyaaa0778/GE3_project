#pragma once

class Camera;
class DirectXCommon;
class Input;
class Object3dRenderer;
class ShaderResourceViewManager;
class SoundManager;
class SpriteRenderer;
class WinApp;

class GameManager {
public:
  void Initialize();

  void Update();

  void Draw();

  void Finalize();

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
};
