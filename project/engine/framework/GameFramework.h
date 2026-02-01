#pragma once

#include <fstream>
#include <memory>

#include "D3DResourceLeakChecker.h"

class Camera;
class SoundManager;

class GameFramework {
public:
  GameFramework();
  virtual ~GameFramework();

  /// <summary>
  /// 実行
  /// </summary>
  void Execute();

  /// <summary>
  /// 初期化
  /// </summary>
  virtual void Initialize();
  /// <summary>
  /// 更新
  /// </summary>
  virtual void Update();
  /// <summary>
  /// 描画
  /// </summary>
  virtual void Draw() = 0;
  /// <summary>
  /// 終了
  /// </summary>
  virtual void Finalize();

  /// <summary>
  /// 終了チェック
  /// </summary>
  /// <returns></returns>
  virtual bool IsEndRequest() { return endRequest_; }

private:
  /// <summary>
  /// ログシステムの初期化
  /// </summary>
  void InitializeLogSystem();

  /// <summary>
  /// 描画前処理
  /// </summary>
  void BeginFrame();
  /// <summary>
  /// 描画後処理
  /// </summary>
  void EndFrame();

private:
  // SoundManagerのポインタ
  std::unique_ptr<SoundManager> soundManager_ = nullptr;
  // Cameraのポインタ
  std::unique_ptr<Camera> camera_ = nullptr;

  // リークチェック
  D3DResourceLeakChecker leakCheck_;

  // ログ
  std::ofstream logStream_;

  // ゲーム終了フラグ
  bool endRequest_ = false;
};
