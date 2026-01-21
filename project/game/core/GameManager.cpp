#include "GameManager.h"

#include "SceneManager.h"
#include"IScene.h"
#include"TitleScene.h"

void GameManager::Initialize() {
  // 基底クラスの初期化
  GameFramework::Initialize();

  sceneManager_ = new SceneManager();

  IScene *scene = new TitleScene();
  //シーンマネージャに最初のシーンをセット
  sceneManager_->SetNextScene(scene);
}

void GameManager::Update() {

  // 基底クラスの更新
  GameFramework::Update();

  sceneManager_->Update();
}

void GameManager::Draw() { sceneManager_->Draw(); }

void GameManager::Finalize() {
  // シーンマネージャを解放
  delete sceneManager_;

  // 基底クラスの終了処理
  GameFramework::Finalize();

  CoUninitialize();
}
