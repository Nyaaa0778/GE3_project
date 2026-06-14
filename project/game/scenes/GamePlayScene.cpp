#include "GamePlayScene.h"

#include <MyEngine.h>

#include "LevelLoader.h"
#include "MathUtility.h"
#include "LightManager.h"

GamePlayScene::GamePlayScene() = default;
GamePlayScene::~GamePlayScene() = default;

void GamePlayScene::Initialize() {
	// カメラのインスタンス生成
	camera_ = std::make_unique<Camera>();

	// ------------------------------------
	// レベルデータのロード
	// ------------------------------------

	LevelLoader loader;
	std::unique_ptr<LevelData> levelData(loader.Load("TL1Sample"));

	// レベルオブジェクトの初期化
	level_ = std::make_unique<Level>();
	level_->Initialize(levelData.get(), camera_.get());

	// ------------------------------------
	// カメラ
	// ------------------------------------

	level_->ApplyCameraParameters(camera_.get());
	camera_->CalculateMatrix();
	camera_->CreateConstantBuffer();

	// ------------------------------------
	// ライト
	// ------------------------------------
	level_->ApplyLightParameters();

	// ------------------------------------
	// 自機
	// ------------------------------------
	
	// モデル
	playerModel_ = std::make_unique<Object3d>();
	playerModel_->Initialize("sphere");
	playerModel_->SetCamera(camera_.get());

	// スポーナーからパラメータを1行で取得し、プレイヤーを初期化
	const LevelData::SpawnerData* spawner = level_->GetSpawner("PlayerSpawn");

	// インスタンス生成
	player_ = std::make_unique<Player>();
	// 初期化
	player_->Initialize(spawner->translation, playerModel_.get(), camera_.get());
	player_->GetWorldTransform()->rotation = spawner->rotation;
	player_->GetWorldTransform()->scale = spawner->scaling;
}

void GamePlayScene::Update() {
	// ------------------------------------
	// ImGui
	// ------------------------------------

#ifdef USE_IMGUI

	UpdateImGui();

#endif

	// ------------------------------------
	// カメラ
	// ------------------------------------

	if (camera_) {
		camera_->CalculateMatrix();
	}

	// ------------------------------------
	// 自機
	// ------------------------------------

	player_->Update();

	// ------------------------------------
	// オブジェクト
	// ------------------------------------

	level_->Update();
}

void GamePlayScene::Draw() {
	// ------------------------------------
	// オブジェクト
	// ------------------------------------

	level_->Draw();

	// ------------------------------------
	// 自機
	// ------------------------------------

	player_->Draw();
}

void GamePlayScene::Finalize() {}

void GamePlayScene::UpdateImGui() {
	ImGui::Begin("Window");

	// ─────────────────────
	// Player Object
	// ─────────────────────

	ImGui::SeparatorText("Player");

	{
		Vector3 pos = player_->GetWorldTransform()->translation;
		if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
			player_->GetWorldTransform()->translation = pos;
		}
	}

	{
		Vector3 scale = player_->GetWorldTransform()->scale;
		if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, -10.0f, 10.0f)) {
			player_->GetWorldTransform()->scale = scale;
		}
	}

	{
		Vector3 rot = player_->GetWorldTransform()->rotation;
		if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f, -6.28f, 6.28f)) {
			player_->GetWorldTransform()->rotation = rot;
		}
	}

	{
		Vector4 color = playerModel_->GetColor();
		float col[4] = {color.x, color.y, color.z, color.w};

		// ImGui カラーピッカー
		if (ImGui::ColorEdit4("Color", col)) {
			Vector4 newColor(col[0], col[1], col[2], col[3]);
			playerModel_->SetColor(newColor);
		}
	}

	// ─────────────────────
	// カメラ
	// ─────────────────────

	ImGui::SeparatorText("Camera");

	// 位置
	{
		Vector3 pos = camera_->GetTranslate();
		if (ImGui::DragFloat3("Camera Position", &pos.x, 0.1f)) {
			camera_->SetTranslate(pos);
		}
	}

	// 回転
	{
		Vector3 rot = camera_->GetRotate();
		if (ImGui::DragFloat3("Camera Rotation", &rot.x, 0.01f)) {
			camera_->SetRotate(rot);
		}
	}

	ImGui::End();
}
