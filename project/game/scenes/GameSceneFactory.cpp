#include "GameSceneFactory.h"

#include "GamePlayScene.h"
#include "TitleScene.h"

IScene *GameSceneFactory::CreateScene(const std::string &sceneName) {

  // 次のシーン生成
  IScene *newScene = nullptr;

  if (sceneName == "TITLE") {
    newScene = new TitleScene();
  } else if (sceneName == "GAMEPLAY") {
    newScene = new GamePlayScene();
  }

  return newScene;
}
