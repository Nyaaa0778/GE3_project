#pragma once

#include "ISceneFactory.h"

class GameSceneFactory : public ISceneFactory {
public:
  IScene *CreateScene(const std::string &sceneName) override;
};
