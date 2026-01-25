#pragma once

#include "IScene.h"
#include <string>

class ISceneFactory {
public:
  virtual ~ISceneFactory() = default;
  //シーン生成
  virtual IScene *CreateScene(const std::string &sceneName) = 0;
};
