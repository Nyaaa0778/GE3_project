#pragma once

#include "D3DResourceLeakChecker.h"

class Camera;
class SoundManager;

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
  // 描画前処理
  void BeginFrame();
  // 描画後処理
  void EndFrame();

private:
  // SoundManagerのポインタ
  SoundManager *soundManager_ = nullptr;
  // Cameraのポインタ
  Camera *camera_ = nullptr;

  // リークチェック
  D3DResourceLeakChecker leakCheck_;

  // ゲーム終了フラグ
  bool endRequest_ = false;
};
