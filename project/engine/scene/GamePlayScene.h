#pragma once
#include "IScene.h"

class Object3d;

class GamePlayScene : public IScene {
public:

  void Initialize() override;

  void Update() override;

  void Draw() override;

  void Finalize() override;

private:
  Object3d *obj_ = nullptr;
};
