#include "TitleScene.h"

#include <MyEngine.h>
#include "LightManager.h"
#include <numbers>

#include "Skybox.h"
#include "DebugCamera.h"
#include "Plane.h"
#include "TextureManager.h"
#include "WireframeObject.h"

TitleScene::TitleScene() = default;
TitleScene::~TitleScene() = default;

void TitleScene::Initialize() {
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({0.5f, 0.0f, 0.0f});
	camera_->SetTranslate({0.0f, 8.0f, -15.0f});
	camera_->CreateConstantBuffer();

	// ② デバッグカメラの初期化（★追加）
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize(); // Inputの取得など
	// 通常カメラの初期位置に合わせる
	debugCamera_->SetRotate(camera_->GetRotate());
	debugCamera_->SetTranslate(camera_->GetTranslate());
	debugCamera_->CalculateMatrix();
	debugCamera_->CreateConstantBuffer();

	// ワイヤーフレームBoxの初期化
	wireBox_ = std::make_unique<WireframeObject>();
	wireBox_->Initialize();
	wireBox_->SetCamera(camera_.get());
	wireBox_->CreateBox({4.0f, 4.0f, 4.0f}); // サイズ4の立方体
	wireBox_->SetPosition({0.0f, 2.0f, 0.0f});
	wireBox_->SetColor({0.0f, 1.0f, 0.5f, 1.0f}); // エメラルドグリーン
}

void TitleScene::Update() {
	auto input = Input::GetInstance();

	if (useDebugCamera_) {
		// ★ debugCameraController_ に camera_ を「操作してくれ」と頼む
		debugCamera_->Update(camera_.get());
		camera_->CalculateMatrix(); // 操作後に行列を更新
	} else {
		// 通常時のカメラ挙動（固定やパス移動など）
		camera_->CalculateMatrix();
	}

	// ワイヤーフレームBoxの回転・更新
	if (wireBox_) {
		wireBox_->Update();
	}

	// --- 1. シーン遷移判定 ---
	if (input->TriggerKey(DIK_SPACE) || input->TriggerButton(XINPUT_GAMEPAD_A)) {
		// 遷移時に振動を止める（重要）
		input->SetShake(0.0f, 0.0f);
		SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
		return;
	}
}

void TitleScene::Draw() {
	if (wireBox_) {
		wireBox_->Draw();
	}
}

void TitleScene::Finalize() {
	wireBox_.reset();
}

void TitleScene::UpdateImGui() {
#ifdef USE_IMGUI
#endif
}
