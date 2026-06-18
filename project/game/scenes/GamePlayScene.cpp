#include "GamePlayScene.h"

#include <MyEngine.h>

#include "LevelLoader.h"
#include "Level.h"

#include "MathUtility.h"
#include "LightManager.h"

#include "Player.h"
#include "PlayerBullet.h"
#include "EnemyBase.h"
#include "RailCameraController.h"
#include "Skydome.h"
#include "RusherEnemy.h"
#include "Collider.h"

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

	// レールカメラ
	railCamera_ = std::make_unique<RailCameraController>();
	railCamera_->Initialize(camera_.get());

	// デバッグカメラ
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize();
	debugCamera_->SetRotate(camera_->GetRotate());
	debugCamera_->SetTranslate(camera_->GetTranslate());
	debugCamera_->CalculateMatrix();
	debugCamera_->CreateConstantBuffer();

	// ------------------------------------
	// ライト
	// ------------------------------------
	level_->ApplyLightParameters();

	// 平行光源を設定
	LightManager* lightManager = LightManager::GetInstance();
	lightManager->SetDirectionalLightColor({1.0f, 1.0f, 1.0f, 1.0f});
	lightManager->SetDirectionalLightDirection({0.0f, -1.0f, 0.5f});
	lightManager->SetDirectionalLightIntensity(1.0f);

	// ------------------------------------
	// 自機
	// ------------------------------------
	
	// モデル
	playerModel_ = std::make_unique<Object3d>();
	playerModel_->Initialize("sphere");
	playerModel_->SetCamera(camera_.get());
	playerModel_->SetLightingType(LightingType::kHalfLambert);

	// スポーナーからパラメータを1行で取得し、プレイヤーを初期化
	const LevelData::SpawnerData* spawner = level_->GetSpawner("PlayerSpawn");

	// インスタンス生成
	player_ = std::make_unique<Player>();
	// 初期化
	player_->Initialize(spawner->translation, playerModel_.get(), camera_.get());
	player_->GetWorldTransform()->rotation = spawner->rotation;
	player_->GetWorldTransform()->scale = spawner->scaling;
	player_->SetParent(railCamera_->GetWorldTransform());
	player_->GetWorldTransform()->translation.z = 20.0f;

	// ------------------------------------
	// 敵
	// ------------------------------------

	// モデル
	enemyModel_ = std::make_unique<Object3d>();
	enemyModel_->Initialize("cube");
	enemyModel_->SetCamera(camera_.get());
	enemyModel_->SetLightingType(LightingType::kHalfLambert);

	// テスト用エネミー生成 (プレイヤーの少し前方)
	auto enemy = std::make_unique<RusherEnemy>();
	Vector3 enemyPos = { 0.0f, 0.0f, 50.0f }; // プレイヤーが z=20.0f に配置されるため
	enemy->Initialize(enemyModel_.get(), camera_.get(), enemyPos, player_.get());
	enemies_.push_back(std::move(enemy));

	// ------------------------------------
	// 天球
	// ------------------------------------

	// モデル
	skydomeModel_ = std::make_unique<Object3d>();
	skydomeModel_->Initialize("skydome");
	skydomeModel_->SetCamera(camera_.get());

	skydome_ = std::make_unique<Skydome>();
	skydome_->Initialize(skydomeModel_.get());
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

	if (useDebugCamera_) {
		debugCamera_->Update(camera_.get());
	} else {
		if (railCamera_) {
			railCamera_->Update();
		} else if (camera_) {
			camera_->CalculateMatrix();
		}
	}

	// ------------------------------------
	// 自機
	// ------------------------------------

	player_->Update();

	// ------------------------------------
	// 敵
	// ------------------------------------

	for (auto& enemy : enemies_) {
		enemy->Update();
	}

	// 衝突判定を実行
	CheckAllCollisions();

	// 死亡した敵をリストから除外 (isAlive_がfalseのものを削除)
	enemies_.remove_if([](const std::unique_ptr<EnemyBase>& enemy) {
		return !enemy->IsAlive();
	});

	// ------------------------------------
	// オブジェクト
	// ------------------------------------

	level_->Update();

	// ------------------------------------
	// 天球
	// ------------------------------------

	skydome_->Update();
}

void GamePlayScene::Draw() {
	// ------------------------------------
	// 天球
	// ------------------------------------

	skydome_->Draw();

	// ------------------------------------
	// オブジェクト
	// ------------------------------------

	level_->Draw();

	// ------------------------------------
	// 自機
	// ------------------------------------

	player_->Draw();

	// ------------------------------------
	// 敵
	// ------------------------------------

	for (auto& enemy : enemies_) {
		enemy->Draw();
	}
}

void GamePlayScene::Finalize() {}

void GamePlayScene::CheckAllCollisions() {
	// プレイヤーと敵の衝突判定
	for (auto& enemy : enemies_) {
		if (Collision::CheckCollision(player_.get(), enemy.get())) {
			player_->OnCollision();
			enemy->OnCollision();
		}
	}

	// プレイヤーの弾と敵の衝突判定
	const auto& bullets = player_->GetBullets();
	for (const auto& bullet : bullets) {
		for (auto& enemy : enemies_) {
			if (Collision::CheckCollision(bullet.get(), enemy.get())) {
				bullet->OnCollision();
				enemy->OnCollision();
			}
		}
	}
}

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

	if (ImGui::Checkbox("Use Debug Camera", &useDebugCamera_)) {
		if (useDebugCamera_) {
			debugCamera_->SetRotate(camera_->GetRotate());
			debugCamera_->SetTranslate(camera_->GetTranslate());
			debugCamera_->CalculateMatrix();
		}
	}

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
