#include "TitleScene.h"

#include "Input.h"
#include "SceneManager.h"

#include "Object3d.h"

#include <stdio.h>

TitleScene::TitleScene() = default;
TitleScene::~TitleScene() = default;

void TitleScene::Initialize() {
  // object3dの初期化
  obj_ = std::make_unique<Object3d>();
  obj_->Initialize("plane");
}

void TitleScene::Update() {
  auto input = Input::GetInstance();

  // --- 1. シーン遷移判定 ---
  if (input->TriggerKey(DIK_RETURN) || input->TriggerButton(XINPUT_GAMEPAD_A)) {
    // 遷移時に振動を止める（重要）
    input->SetShake(0.0f, 0.0f);
    SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
    return;
  }

  // --- 2. 押し込み判定と振動の処理 ---
  float speed = 0.1f; // 通常速度

  // 左スティック押し込み (L3) を判定
  if (input->PushButton(XINPUT_GAMEPAD_LEFT_THUMB)) {
    speed = 0.3f; // ダッシュ速度
    // 振動させる（左モーター：低周波でガタガタ、右モーター：高周波で細かく）
    input->SetShake(0.3f, 0.7f);
  } else {
    // 離していたら振動を止める
    input->SetShake(0.0f, 0.0f);
  }

if (input->TriggerButton(XINPUT_GAMEPAD_RIGHT_THUMB)) {
    input->SetShake(1.0f, 1.0f, 3.0f); // 1秒間ドカン！と震えて勝手に止まる
  }
  // --- 3. スティック移動処理 ---
  Input::Stick lStick = input->GetLeftStick();
  Vector3 pos = obj_->GetPosition();
  pos.x += lStick.x * speed;
  pos.y += lStick.y * speed;
  obj_->SetPosition(pos);

  obj_->Update();
}

void TitleScene::Draw() { obj_->Draw(); }

void TitleScene::Finalize() {}
