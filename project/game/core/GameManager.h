#pragma once

#include <memory>

#include "GameFramework.h"

class ISceneFactory;

class GameManager : public GameFramework {
public:
  /// <summary>
  /// コンストラクタ
  /// </summary>
  GameManager();
  /// <summary>
  /// デストラクタ
  /// </summary>
  ~GameManager() override;

  // 初期化
  void Initialize() override;
  // 更新
  void Update() override;
  // 描画
  void Draw() override;
  // 終了
  void Finalize() override;

private:
  // シーンファクトリー
  std::unique_ptr<ISceneFactory> sceneFactory_ = nullptr;

private:
  // ゲーム終了フラグ
  bool endRequest_ = false;
};
