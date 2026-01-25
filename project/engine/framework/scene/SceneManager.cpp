#include "SceneManager.h"

#include "IScene.h"
#include "ISceneFactory.h"

#include <cassert>

//================================================================================
// シングルトン
//================================================================================

SceneManager *SceneManager::instance = nullptr;

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>SceneManagerの唯一のインスタンス</returns>
SceneManager *SceneManager::GetInstance() {
  if (instance == nullptr) {
    instance = new SceneManager;
  }

  return instance;
}

void SceneManager::Shutdown() {
  delete instance;
  instance = nullptr;
}

SceneManager::SceneManager() {
  // 初期シーン
  scene_ = nullptr;
  nextScene_ = nullptr;
}

SceneManager::~SceneManager() {
  // 最後のシーンの終了と解放
  if (scene_) {
    scene_->Finalize();
    delete scene_;
  }

  if (nextScene_) {
    delete nextScene_;
    nextScene_ = nullptr;
  }
}

void SceneManager::Update() {
  // シーンの切り替え
  ChangeSceneInternal();

  if (!scene_) {
    return; // まだ何もないなら何もしない
  }

  // 実行中のシーンを更新
  scene_->Update();
}

void SceneManager::Draw() {
  if (!scene_) {
    return; // まだ何もないなら何もしない
  }

  scene_->Draw();
}

void SceneManager::ChangeScene(const std::string &sceneName) {
  assert(sceneFactory_ && "SceneFactory is not set");

  assert(nextScene_ == nullptr);

  //次のシーンを予約
  nextScene_ = sceneFactory_->CreateScene(sceneName);
}

void SceneManager::ChangeSceneInternal() {
  // 次のシーンの予約があるとき
  if (nextScene_) {
    // 旧シーンの終了
    if (scene_) {
      scene_->Finalize();
      delete scene_;
    }

    // シーンの切り替え
    scene_ = nextScene_;
    nextScene_ = nullptr;

    // シーンマネージャをセット
    scene_->SetSceneManager(this);

    // 次のシーンを初期化
    scene_->Initialize();
  }
}
