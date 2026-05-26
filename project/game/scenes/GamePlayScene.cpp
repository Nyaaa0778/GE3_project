#include "GamePlayScene.h"

#include <MyEngine.h>
#include <Random.h>

#include "Player.h"
#include "RusherEnemy.h"
#include "RailCameraController.h"
#include "Skybox.h"

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

	// レールカメラの初期化
	railCameraController_ = std::make_unique<RailCameraController>();
	railCameraController_->Initialize(camera_.get(), kInitialCameraPos);

	// -----------------------
	// 自機の初期化
	// -----------------------

	// モデル
	playerModel_ = std::make_unique<Object3d>();
	playerModel_->Initialize("player");

	// 弾（複数弾）のモデル
	playerBulletModel_ = std::make_unique<Object3d>();
	playerBulletModel_->Initialize("bullet");

	// 親子関係の設定：プレイヤーのモデルの親にレールのWorldTransformをセット
	playerModel_->GetWorldTransform().parent = &railCameraController_->GetWorldTransform();

	// 実体生成（プレイヤー自身の初期位置はレールに対するローカル座標なので、原点付近にする）
	player_ = std::make_unique<Player>();
	player_->Initialize(camera_.get(), {0.0f, 0.0f, 0.0f}, playerModel_.get(), "bullet");

	// -----------------------
	// 敵の初期化：最大数だけ最初からスポーン
	// -----------------------
	enemies_.clear();
	for (int i = 0; i < kMaxEnemyCount; ++i) {
		SpawnEnemy();
	}

	// -----------------------
	// 天球の初期化
	// -----------------------

	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize("resources/sprites/rostock_laage_airport_4k.dds", camera_.get());
}

void GamePlayScene::SpawnEnemy() {
	EnemyStatus status = {
		10,    // maxHp
		10,    // currentHp
		1,     // attackPower
		0.1f   // speed
	};

	// ランダムなX座標でスポーン（Y・Zは固定）
	Vector3 spawnPos = {
		Random::RangeFloat(-kSpawnRangeX, kSpawnRangeX),
		kSpawnRangeY,
		kSpawnZ
	};

	auto enemy = std::make_unique<RusherEnemy>(status);

	enemy->Initialize(camera_.get(), spawnPos, kEnemyModelName);

	// ★生成直後に行列と定数バッファを更新して、描画時に原点に表示されるのを防ぐ
	if (player_) {
		enemy->Update(player_.get());
	}

	enemies_.push_back(std::move(enemy));
}

void GamePlayScene::Update() {

	// ImGuiの描画
	UpdateImGui();

	// -----------------------
	// レールカメラの更新（自動スクロール＆カメラの座標設定）
	// -----------------------
	railCameraController_->Update();

	// -----------------------
	// カメラの更新
	// -----------------------

	if (useDebugCamera_) {
		debugCamera_->Update(camera_.get());
	} else {
		camera_->CalculateMatrix();
	}

	// -----------------------
	// 自機の更新
	// -----------------------

	player_->Update(railCameraController_->GetPosition());

	// -----------------------
	// 敵の更新
	// -----------------------

	// 全敵を更新
	for (auto& enemy : enemies_) {
		if (enemy) {
			enemy->Update(player_.get());
		}
	}

	// 死亡した敵を除去 → 不足分をスポーン
	// ★ erase-remove イディオム：死亡済みを一括削除
	enemies_.erase(
		std::remove_if(enemies_.begin(), enemies_.end(),
					   [](const std::unique_ptr<RusherEnemy>& e) {
						   return e == nullptr || !e->IsAlive();
					   }),
		enemies_.end()
	);

	// ★ 削除後に補充する（Update の末尾でスポーンするのが重要）
	//    → SpawnEnemy() → Initialize() で pos_ と SetPosition() が確定してから
	//      次フレームの Draw が呼ばれるため、チカつきが発生しない
	while (static_cast<int>(enemies_.size()) < kMaxEnemyCount) {
		SpawnEnemy();
	}
}

void GamePlayScene::Draw() {
	// -----------------------
	// 自機の描画
	// -----------------------

	player_->Draw();

	// -----------------------
	// 敵の描画
	// -----------------------
	for (auto& enemy : enemies_) {
		if (enemy) {
			enemy->Draw();
		}
	}

	// -----------------------
	// 天球の描画
	// -----------------------

	skybox_->Draw();
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