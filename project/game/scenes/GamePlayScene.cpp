#include "GamePlayScene.h"

#include <MyEngine.h>

#include "Player.h"

GamePlayScene::GamePlayScene() = default;
GamePlayScene::~GamePlayScene() = default;

void GamePlayScene::Initialize() {
	// -----------------------
	// カメラの初期化
	// -----------------------

	camera_ = std::make_unique<Camera>();
	camera_->SetTranslate(kInitialCameraPos);

	// デバッグカメラ
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize();
	// 通常カメラの初期位置に合わせる
	debugCamera_->SetTranslate(camera_->GetTranslate());
	debugCamera_->CalculateMatrix();

	// -----------------------
	// 自機の初期化
	// -----------------------

	// モデル
	playerModel_ = std::make_unique<Object3d>();
	playerModel_->Initialize("player");
	// 実体生成
	player_ = std::make_unique<Player>();
	player_->Initialize(camera_.get(), kInitialPlayerPos, playerModel_.get());
}

void GamePlayScene::Update() {

	// ImGuiの描画
	UpdateImGui();

	// -----------------------
	// カメラの更新
	// -----------------------

	if (useDebugCamera_) {
		// debugCameraController_ に camera_ を「操作してくれ」と頼む
		debugCamera_->Update(camera_.get());
	} else {
		// 通常時のカメラ挙動（固定やパス移動など）
		camera_->CalculateMatrix();
	}

	// -----------------------
	// 自機の更新
	// -----------------------

	player_->Update();
}

void GamePlayScene::Draw() {
	// -----------------------
	// 自機の描画
	// -----------------------

	player_->Draw();
}

void GamePlayScene::Finalize() {}

void GamePlayScene::UpdateImGui() {
	ImGui::Begin("Debug Window");

	if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		// カメラ切り替え
		ImGui::Checkbox("Use Debug Camera", &useDebugCamera_);

		ImGui::Separator();

		// 1. 操作対象のカメラを決定 (常に操作されているメインカメラを対象にする)
		auto targetCamera = camera_.get();
		ImGui::Text("Controller: %s", useDebugCamera_ ? "Debug Camera" : "Main Camera");

		// 2. ★重要★ 表示する直前に「今のカメラの座標」を必ず取得する
		Vector3 camPos = targetCamera->GetTranslate();

		// 3. ImGuiに最新の値を渡す
		// DragFloat3 は「値が操作された時」に true を返します
		if (ImGui::DragFloat3("Position", &camPos.x, 0.1f)) {
			// 操作された時だけカメラに値を戻す
			targetCamera->SetTranslate(camPos);
		}

		// 回転も同様に「直前に取得」
		Vector3 camRot = targetCamera->GetRotate();
		if (ImGui::DragFloat3("Rotation", &camRot.x, 0.01f)) {
			targetCamera->SetRotate(camRot);
		}

		if (ImGui::Button("Reset Transform")) {
			targetCamera->SetTranslate({0.0f, 0.0f, -10.0f});
			targetCamera->SetRotate({0.0f, 0.0f, 0.0f});
		}
	}

	ImGui::End();
}