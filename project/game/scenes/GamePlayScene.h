#pragma once

#include "IScene.h"

#include <memory>

class Object3d;
class Camera;

class GamePlayScene : public IScene {
public:
  GamePlayScene();
  ~GamePlayScene();

  void Initialize() override;

  void Update() override;

  void Draw() override;

  void Finalize() override;

private:
  // モデル
  std::unique_ptr<Object3d> obj_;
};
