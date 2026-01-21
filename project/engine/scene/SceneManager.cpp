#include "SceneManager.h"

#include "GamePlayScene.h"
#include "IScene.h"
#include "TitleScene.h"

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
  scene_ = new TitleScene();

  scene_->SetSceneManager(this);

  scene_->Initialize();
}

SceneManager::~SceneManager() {
  // 最後のシーンの終了と解放
  scene_->Finalize();
  delete scene_;
}

void SceneManager::Update() {
  // シーンの切り替え
  ChangeScene();

  // 実行中のシーンを更新
  scene_->Update();
}

void SceneManager::Draw() { scene_->Draw(); }

void SceneManager::ChangeScene() {
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
