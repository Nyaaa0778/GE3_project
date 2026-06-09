#include "GamePlayScene.h"

#include <MyEngine.h>
#include "LevelLoader.h"
#include "MathUtility.h"
#include "LightManager.h"
#include "../Player.h"
#include "../Enemy.h"
#include "../DebugNetwork.h"

#include <fstream>

GamePlayScene::GamePlayScene() = default;
GamePlayScene::~GamePlayScene() = default;

void GamePlayScene::Initialize() {
	// カメラのインスタンス生成
	camera_ = std::make_unique<Camera>();

	// レベルデータのロード
	LevelLoader loader;
	std::unique_ptr<LevelData> levelData(loader.Load("TL1Sample"));

	// ロードしたカメラパラメータ（位置・回転）があれば設定、なければデフォルト値
	if (!levelData->cameras.empty()) {
		camera_->SetTranslate(levelData->cameras[0].translation);
		camera_->SetRotate(levelData->cameras[0].rotation);
	} else {
		camera_->SetRotate({ 0.3f, 0.0f, 0.0f });
		camera_->SetTranslate({ 0.0f, 6.0f, -20.0f });
	}
	camera_->CalculateMatrix();
	camera_->CreateConstantBuffer();

	// ロードしたライトパラメータ（位置・回転）があれば設定
	if (!levelData->lights.empty()) {
		const auto& lightData = levelData->lights[0];

		// 点光源（Local Light）の設定
		LightManager::GetInstance()->SetLocalLightPosition(lightData.translation);
		LightManager::GetInstance()->SetLocalLightIntensity(1.0f);
		LightManager::GetInstance()->SetLocalLightDistance(50.0f); // 届く範囲を拡張

		// 平行光源（Directional Light）は使用しないため無効化（強度 0.0f）
		LightManager::GetInstance()->SetDirectionalLightIntensity(0.0f);
	}

	// ネットワークの初期化
	DebugNetwork::GetInstance()->Initialize(12345);

	// レベルの配置初期ロード
	ReloadLevel();
}

void GamePlayScene::ReloadLevel() {
	// 既存キャラクターおよびオブジェクトのクリア
	objects_.clear();
	player_.reset();
	enemy_.reset();
	frameLogs_.clear();
	currentFrameIndex_ = 0;
	isRewindMode_ = false;

	LevelLoader loader;
	std::unique_ptr<LevelData> levelData(loader.Load("TL1Sample"));

	// 読み込んだレベルデータのオブジェクトを生成・初期化
	for (const auto& objectData : levelData->objects) {
		if (objectData.filename.empty()) {
			continue;
		}
		auto newObj = std::make_unique<Object3d>();
		newObj->Initialize(objectData.filename);
		newObj->SetPosition(objectData.translation);
		newObj->SetRotation(objectData.rotation);
		newObj->SetScale(objectData.scaling);
		newObj->SetCamera(camera_.get());
		objects_.push_back(std::move(newObj));
	}

	// Spawnerデータから初期配置を設定
	Vector3 playerPos = { -0.6439096927642822f, 0.0f, -0.04999999701976776f };
	Vector3 playerRot = { 0.0f, 0.0f, 0.0f };
	Vector3 enemyPos = { 5.0799946784973145f, 0.0f, 7.99898624420166f };
	Vector3 enemyRot = { 0.0f, 0.0f, 0.0f };

	for (const auto& spawner : levelData->spawners) {
		if (spawner.entityType.find("Player") != std::string::npos) {
			playerPos = spawner.translation;
			playerRot = spawner.rotation;
		}
		else if (spawner.entityType.find("Enemy") != std::string::npos) {
			enemyPos = spawner.translation;
			enemyRot = spawner.rotation;
		}
	}

	// キャラクター初期配置
	player_ = std::make_unique<Player>();
	player_->Initialize(camera_.get(), playerPos);
	player_->SetRotation(playerRot);
	player_->SetSpeed(playerSpeed_);

	enemy_ = std::make_unique<Enemy>();
	enemy_->Initialize(camera_.get(), enemyPos);
	enemy_->SetRotation(enemyRot);
	enemy_->SetSpeed(enemySpeed_);
	enemy_->SetWaitTime(enemyWaitTime_);
	enemy_->SetSeed(enemySeed_);
}

void GamePlayScene::SavePlayLog() {
	nlohmann::json root;
	root["frames"] = frameLogs_;

	std::filesystem::create_directory("logs");
	std::ofstream file("logs/play_log.json");
	if (file.is_open()) {
		file << root.dump(4);
		file.close();
	}
}

void GamePlayScene::Update() {
	// ネットワーク受信処理
	int receivedFrame = DebugNetwork::GetInstance()->UpdateReceive();
	if (receivedFrame >= 0) {
		isRewindMode_ = true;
		if (receivedFrame < (int)frameLogs_.size()) {
			currentFrameIndex_ = receivedFrame;

			const auto& frameData = frameLogs_[currentFrameIndex_];
			
			// プレイヤー位置復元
			Vector3 pPos = {
				frameData["player"]["translation"][0].get<float>(),
				frameData["player"]["translation"][1].get<float>(),
				frameData["player"]["translation"][2].get<float>()
			};
			Vector3 pRot = {
				frameData["player"]["rotation"][0].get<float>(),
				frameData["player"]["rotation"][1].get<float>(),
				frameData["player"]["rotation"][2].get<float>()
			};
			player_->SetPosition(pPos);
			player_->SetRotation(pRot);

			// 敵位置復元
			if (frameData.contains("enemies") && !frameData["enemies"].empty()) {
				const auto& enemyData = frameData["enemies"][0];
				Vector3 ePos = {
					enemyData["translation"][0].get<float>(),
					enemyData["translation"][1].get<float>(),
					enemyData["translation"][2].get<float>()
				};
				Vector3 eRot = {
					enemyData["rotation"][0].get<float>(),
					enemyData["rotation"][1].get<float>(),
					enemyData["rotation"][2].get<float>()
				};
				enemy_->SetPosition(ePos);
				enemy_->SetRotation(eRot);
				enemy_->SetHp(enemyData["hp"].get<float>());
			}
		}
	}

	bool takeoverTriggered = false;

#ifdef USE_IMGUI
	ImGui::Begin("Window");

	// ─────────────────────
	// 状態表示と制御
	// ─────────────────────
	ImGui::SeparatorText("Debug Mode");
	if (isRewindMode_) {
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Mode: REWIND / PAUSED (Blender Sync)");
		ImGui::Text("Frame: %d / %d", currentFrameIndex_, (int)frameLogs_.size() - 1);
		if (ImGui::Button("Takeover from Current Frame (制御再開) [Enter]")) {
			takeoverTriggered = true;
		}
	} else {
		ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Mode: NORMAL PLAY");
		ImGui::Text("Frame: %d (Logging...)", currentFrameIndex_);
	}

	if (ImGui::Button("Reload Level JSON (配置再読込)")) {
		ReloadLevel();
	}
	ImGui::SameLine();
	if (ImGui::Button("Save Play Log (ログ保存)")) {
		SavePlayLog();
	}
	ImGui::SameLine();
	if (ImGui::Button("Trigger Bug Flag (バグ発生)")) {
		triggerBug_ = true;
	}

	// ─────────────────────
	// キャラクターパラメータ
	// ─────────────────────
	ImGui::SeparatorText("Player Parameters");
	if (ImGui::DragFloat("Player Speed", &playerSpeed_, 0.01f, 0.0f, 2.0f)) {
		player_->SetSpeed(playerSpeed_);
	}

	ImGui::SeparatorText("Enemy Parameters");
	if (ImGui::DragFloat("Enemy Speed", &enemySpeed_, 0.005f, 0.0f, 1.0f)) {
		enemy_->SetSpeed(enemySpeed_);
	}
	if (ImGui::DragInt("Enemy Wait Time", &enemyWaitTime_, 1, 0, 600)) {
		enemy_->SetWaitTime(enemyWaitTime_);
	}
	if (ImGui::DragInt("Enemy Random Seed", &enemySeed_, 1, 0, 9999)) {
		enemy_->SetSeed(enemySeed_);
	}

	// ─────────────────────
	// カメラ
	// ─────────────────────
	ImGui::SeparatorText("Camera");
	{
		Vector3 pos = camera_->GetTranslate();
		if (ImGui::DragFloat3("Camera Position", &pos.x, 0.1f)) {
			camera_->SetTranslate(pos);
		}
	}
	{
		Vector3 rot = camera_->GetRotate();
		if (ImGui::DragFloat3("Camera Rotation", &rot.x, 0.01f)) {
			camera_->SetRotate(rot);
		}
	}

	ImGui::End();
#endif

	// キー入力によるテイクオーバー確認
	if (isRewindMode_ && (Input::GetInstance()->TriggerKey(DIK_RETURN) || takeoverTriggered)) {
		isRewindMode_ = false;
		// ログを現在フレームまで切り詰め
		if (currentFrameIndex_ < (int)frameLogs_.size()) {
			frameLogs_.resize(currentFrameIndex_ + 1);
		}
	}

	// キャラクターの更新
	player_->Update(isRewindMode_);
	enemy_->Update(isRewindMode_);

	// ログ書き出し
	if (!isRewindMode_) {
		nlohmann::json frameJson;
		frameJson["frame"] = currentFrameIndex_;

		// プレイヤー情報
		nlohmann::json pJson;
		pJson["translation"] = { player_->GetPosition().x, player_->GetPosition().y, player_->GetPosition().z };
		pJson["rotation"] = { player_->GetRotation().x, player_->GetRotation().y, player_->GetRotation().z };
		frameJson["player"] = pJson;

		// 敵情報
		nlohmann::json eList = nlohmann::json::array();
		nlohmann::json eJson;
		eJson["index"] = 0;
		eJson["translation"] = { enemy_->GetPosition().x, enemy_->GetPosition().y, enemy_->GetPosition().z };
		eJson["rotation"] = { enemy_->GetRotation().x, enemy_->GetRotation().y, enemy_->GetRotation().z };
		eJson["hp"] = enemy_->GetHp();
		eJson["anim_state"] = (enemy_->GetState() == Enemy::State::Wander) ? "Wander" : "Wait";
		eList.push_back(eJson);
		frameJson["enemies"] = eList;

		// イベントフラグ
		nlohmann::json evtJson;
		evtJson["bug_trigger"] = triggerBug_;
		evtJson["msg"] = triggerBug_ ? "BUG TRIGGERED BY USER" : "";
		frameJson["events"] = evtJson;

		// 軌跡描画用ノード
		nlohmann::json nodes = nlohmann::json::array();
		nodes.push_back({ player_->GetPosition().x, player_->GetPosition().y, player_->GetPosition().z });
		frameJson["thread_nodes"] = nodes;

		frameLogs_.push_back(frameJson);
		triggerBug_ = false; // イベントリセット
		currentFrameIndex_++;
	}

	if (camera_) {
		camera_->CalculateMatrix();
	}

	for (auto& obj : objects_) {
		obj->Update();
	}
}

void GamePlayScene::Draw() {
	// 背景オブジェクト
	for (auto& obj : objects_) {
		obj->Draw();
	}

	// プレイヤー
	player_->Draw();

	// 敵
	enemy_->Draw();
}

void GamePlayScene::Finalize() {
	// ログ保存して終了
	SavePlayLog();
	DebugNetwork::GetInstance()->Finalize();

	objects_.clear();
	player_.reset();
	enemy_.reset();
}
