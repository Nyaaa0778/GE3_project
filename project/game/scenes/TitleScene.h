#pragma once

#include <memory>

#include "IScene.h"

class Object3d;

class TitleScene : public IScene {
public:
  TitleScene();
  ~TitleScene();

  void Initialize() override;

  void Update() override;

  void Draw() override;

  void Finalize() override;

private:
  // モデル
  std::unique_ptr<Object3d> obj_;
};
