#pragma once

#include <memory>

#include "ISceneFactory.h"

class GameSceneFactory : public ISceneFactory {
public:
	std::unique_ptr<IScene> CreateScene(const std::string& sceneName) override;
};
