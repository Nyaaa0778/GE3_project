#include "TitleScene.h"

#include "GamePlayScene.h"
#include "Input.h"
#include "SceneManager.h"

void TitleScene::Initialize() {}

void TitleScene::Update() {
  // Enterキーを押したら
  if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
    // 次のシーン（ゲームプレイシーン）を生成
    IScene *scene = new GamePlayScene();
    // シーン切り替え依頼
    sceneManager_->SetNextScene(scene);
  }
}

void TitleScene::Draw() {}

void TitleScene::Finalize() {}
