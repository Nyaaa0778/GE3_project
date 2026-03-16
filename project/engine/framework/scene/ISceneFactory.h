#pragma once

#include <memory>
#include <string>

#include "IScene.h"

class ISceneFactory {
public:
	virtual ~ISceneFactory() = default;
	// シーン生成
	virtual std::unique_ptr<IScene> CreateScene(const std::string& sceneName) = 0;
};
