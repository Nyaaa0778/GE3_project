#include "GameManager.h"

#include "IScene.h"
#include "GameSceneFactory.h"
#include "SceneManager.h"

void GameManager::Initialize() {
  // 基底クラスの初期化
  GameFramework::Initialize();

  //シーンファクトリを生成してマネージャにセット
  sceneFactory_ = new GameSceneFactory();

  SceneManager::GetInstance()->SetSceneFactory(sceneFactory_);

  // シーンマネージャに最初のシーンをセット
  SceneManager::GetInstance()->ChangeScene("TITLE");
}

void GameManager::Update() {

  // 基底クラスの更新
  GameFramework::Update();

  SceneManager::GetInstance()->Update();
}

void GameManager::Draw() { SceneManager::GetInstance()->Draw(); }

void GameManager::Finalize() {
  // シーンマネージャを解放
  SceneManager::GetInstance()->Shutdown();

  //シーンファクトリを解放
  delete sceneFactory_;

  // 基底クラスの終了処理
  GameFramework::Finalize();

  CoUninitialize();
}
