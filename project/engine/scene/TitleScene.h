#pragma once

#include "IScene.h"

class TitleScene : public IScene {
public:
  void Initialize() override;

  void Update() override;

  void Draw() override;

  void Finalize() override;
};
