#include "SceneManager.h"

#include "IScene.h"
#include "ISceneFactory.h"

#include <cassert>

//================================================================================
// シングルトン
//================================================================================

std::unique_ptr<SceneManager> SceneManager::instance = nullptr;

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>SceneManagerの唯一のインスタンス</returns>
SceneManager *SceneManager::GetInstance() {
  if (instance == nullptr) {
    instance.reset(new SceneManager());
  }

  return instance.get();
}

void SceneManager::Finalize() { instance.reset(); }

SceneManager::SceneManager() {
  // 初期シーン
  scene_ = nullptr;
  nextScene_ = nullptr;
}

SceneManager::~SceneManager() {
  // 最後のシーンの終了と解放
  if (scene_) {
    scene_->Finalize();
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
    if (scene_) {
      scene_->Finalize();
    }

    // 所有権の移動 (古いシーンはここで自動的に delete される)
    scene_ = std::move(nextScene_);

    scene_->SetSceneManager(this);
    scene_->Initialize();
  }
}
