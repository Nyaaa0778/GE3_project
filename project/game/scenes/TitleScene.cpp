#include "TitleScene.h"

#include "Input.h"
#include "SceneManager.h"

void TitleScene::Initialize() {}

void TitleScene::Update() {
  // Enterキーを押したら
  if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
    SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
  }
}

void TitleScene::Draw() {}

void TitleScene::Finalize() {}
