#include "GameSceneFactory.h"

#include "GamePlayScene.h"
#include "TitleScene.h"

std::unique_ptr<IScene>
GameSceneFactory::CreateScene(const std::string& sceneName) {

	if (sceneName == "TITLE") {
		return std::make_unique<TitleScene>();
	} else if (sceneName == "GAMEPLAY") {
		return std::make_unique<GamePlayScene>();
	}

	return nullptr;
}
