#include "GamePlayScene.h"

#include <MyEngine.h>
#include "PostProcessRenderer.h"
#include "TextureManager.h"
#include "Input.h"
#include "GameFramework.h"
#include "Logger.h"
#include "DirectXCommon.h"
#include "ShaderResourceViewManager.h"
#include "../externals/imgui/imgui.h"
#include <filesystem>
#include <unordered_map>
#include <string>
#include <vector>
#include "DevelopEditor.h"

#include "LevelLoader.h"
#include "Level.h"

#include "MathUtility.h"
#include "LightManager.h"

#include "LockOn.h"
#include "Player.h"
#include "IPlayerBullet.h"
#include "EnemyBase.h"
#include "RailCameraController.h"
#include "Skydome.h"
#include "RusherEnemy.h"
#include "Shockwave.h"

#include "Collider.h"
#include "Shake.h"
#include "TimeManager.h"

GamePlayScene::GamePlayScene() = default;
GamePlayScene::~GamePlayScene() = default;

void GamePlayScene::Initialize() {
	// カメラのインスタンス生成
	camera_ = std::make_unique<Camera>();

	// ------------------------------------
	// レベルデータのロード
	// ------------------------------------

	levelFilename_ = "TL1Sample";
	LevelLoader loader;
	levelData_ = loader.Load(levelFilename_);

	// ファイル更新日時を初期記録
	std::string fullpath = "resources/levels/" + levelFilename_ + ".json";
	if (std::filesystem::exists(fullpath)) {
		lastLevelWriteTime_ = std::filesystem::last_write_time(fullpath);
	}

	// レベルオブジェクトの初期化
	level_ = std::make_unique<Level>();
	level_->Initialize(levelData_.get(), camera_.get());

	// ------------------------------------
	// カメラ
	// ------------------------------------

	level_->ApplyCameraParameters(camera_.get());
	camera_->CalculateMatrix();
	camera_->CreateConstantBuffer();

	// レールカメラ
	railCamera_ = std::make_unique<RailCameraController>();
	railCamera_->Initialize(camera_.get(), levelData_->railSpline, levelFilename_);

	// エディタからの配置・保存コールバックを登録
	DevelopEditor* editor = DevelopEditor::GetInstance();
	editor->SetOnPlaceObjectCallback([this](const std::string& assetName) {
		PlaceNewObject(assetName);
	});
	editor->SetOnSaveCallback([this]() {
		SaveLevel();
	});
	editor->SetOnPlaceSpriteCallback([this](const std::string& assetName) {
		PlaceNewSprite(assetName);
	});

	// デバッグカメラ
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize();
	debugCamera_->SetRotate(camera_->GetRotate());
	debugCamera_->SetTranslate(camera_->GetTranslate());
	debugCamera_->CalculateMatrix();
	debugCamera_->CreateConstantBuffer();

#ifdef USE_IMGUI
	prevIsEditorMode_ = DevelopEditor::GetInstance()->IsEditorMode();
	useDebugCamera_ = prevIsEditorMode_;
#endif

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
	// ロックオン
	// ------------------------------------

	lockOn_ = std::make_unique<LockOn>();
	lockOn_->Initialize();
	player_->SetLockOn(lockOn_.get());

	// ------------------------------------
	// 敵
	// ------------------------------------

	// モデル
	enemyModel_ = std::make_unique<Object3d>();
	enemyModel_->Initialize("cube");
	enemyModel_->SetCamera(camera_.get());
	enemyModel_->SetLightingType(LightingType::kHalfLambert);

	// Spawnerデータから "Enemy" という名前が含まれるものをすべて取得して生成
	std::vector<LevelData::SpawnerData> enemySpawners = level_->GetSpawners("Enemy");
	for (const auto& spawnerData : enemySpawners) {
		auto enemy = std::make_unique<RusherEnemy>();
		enemy->Initialize(enemyModel_.get(), camera_.get(), spawnerData.translation, player_.get());
		enemy->GetWorldTransform().rotation = spawnerData.rotation;
		enemy->GetWorldTransform().scale = spawnerData.scaling;

		enemies_.push_back(std::move(enemy));
	}

	// ------------------------------------
	// 天球
	// ------------------------------------

	// モデル
	skydomeModel_ = std::make_unique<Object3d>();
	skydomeModel_->Initialize("skydome");
	skydomeModel_->SetCamera(camera_.get());

	skydome_ = std::make_unique<Skydome>();
	skydome_->Initialize(skydomeModel_.get());

	// 画面シェイク
	shake_ = std::make_unique<Shake>();
	// ノイズテクスチャを事前にロードしてキャッシュしておく
	TextureManager::GetInstance()->LoadTexture("resources/sprites/noise0.png");
	TextureManager::GetInstance()->LoadTexture("resources/sprites/noise1.png");

	// ------------------------------------
	// ゴール初期化
	// ------------------------------------
	goal_ = std::make_unique<Goal>();
	Vector3 goalPos = {0.0f, 0.0f, 150.0f};
	if (railCamera_) {
		const auto& controlPoints = railCamera_->GetControlPoints();
		if (!controlPoints.empty()) {
			goalPos = controlPoints.back();
		}
		// レールカメラのループをオフにしてゴール地点で停止するようにする
		railCamera_->SetIsLoop(false);
	}
	goal_->Initialize(goalPos, camera_.get());

	isGoalReached_ = false;

	// パーティクルグループの作成と初期クリア
	ParticleManager::GetInstance()->CreateParticleGroup("CircleParticle", "resources/sprites/circle.png", ParticleManager::ParticleShape::kPlane);
	ParticleManager::GetInstance()->ClearAllParticles();
}

void GamePlayScene::Update() {
#ifdef USE_IMGUI
	// エディタモードの切り替えを検知してDebugCameraを自動オン/オフ
	bool isEditorMode = DevelopEditor::GetInstance()->IsEditorMode();
	if (isEditorMode != prevIsEditorMode_) {
		if (isEditorMode) {
			useDebugCamera_ = true;
		} else {
			useDebugCamera_ = false;
		}
		prevIsEditorMode_ = isEditorMode;
	}
#endif

#ifdef USE_IMGUI
	// ホットリロードの監視
	std::string levelPath = "resources/levels/" + levelFilename_ + ".json";
	if (std::filesystem::exists(levelPath)) {
		auto currentWriteTime = std::filesystem::last_write_time(levelPath);
		if (currentWriteTime != lastLevelWriteTime_) {
			ReloadLevel();
			lastLevelWriteTime_ = currentWriteTime;
		}
	}
#endif

	// ------------------------------------
	// ImGui
	// ------------------------------------
#ifdef USE_IMGUI
	UpdateImGui();
#endif


	// ------------------------------------
	// カメラ
	// ------------------------------------

	bool isPaused = DevelopEditor::GetInstance()->IsPaused();

	if (railCamera_ && !isPaused) {
		railCamera_->Update(!useDebugCamera_);
	}
	if (useDebugCamera_) {
		debugCamera_->Update(camera_.get());
	}

#ifdef USE_IMGUI
	if (railCamera_ && !DevelopEditor::GetInstance()->IsEditorMode()) {
		railCamera_->DrawDebugSpline();
	}
#endif

	if (!isPaused) {
		// 画面シェイクの更新と適用
		if (shake_) {
			shake_->Update(TimeManager::GetInstance()->GetDeltaTime());
			if (shake_->IsActive() && !useDebugCamera_) {
				Vector3 offset = shake_->GetOffset();
				camera_->matWorld.m[3][0] += offset.x;
				camera_->matWorld.m[3][1] += offset.y;
				camera_->matWorld.m[3][2] += offset.z;

				camera_->matView = MathUtility::MakeInverseMatrix(camera_->matWorld);
				camera_->UpdateViewProjection();
			}
		}

		// ------------------------------------
		// ゴールの更新・アニメーション
		// ------------------------------------
		if (goal_) {
			goal_->Update();
		}

		// ------------------------------------
		// ゴール到達判定とシーン遷移
		// ------------------------------------
		if (isGoalReached_) {
			Input* input = Input::GetInstance();
			if (input->TriggerKey(DIK_RETURN) || input->TriggerButton(XINPUT_GAMEPAD_A)) {
				SceneManager::GetInstance()->ChangeScene("TITLE");
				return;
			}
		} else {
			// 自機との衝突判定によるゴール到達チェック
			if (player_ && goal_) {
				if (Collision::CheckCollision(player_.get(), goal_.get())) {
					isGoalReached_ = true;
					if (railCamera_) {
						railCamera_->SetIsPlaying(false);
					}
				}
			}

			// カメラがレール末尾に到達したことによるゴール到達チェック
			if (railCamera_ && !railCamera_->GetIsLoop()) {
				float maxTime = static_cast<float>((std::max) (0ULL, railCamera_->GetControlPoints().size()) - 1);
				if (railCamera_->GetSplineTime() >= maxTime) {
					isGoalReached_ = true;
					railCamera_->SetIsPlaying(false);
				}
			}
		}

		// ------------------------------------
		// 自機 & 敵 & 衝突判定 (ゴール未到達時のみ更新)
		// ------------------------------------
		if (!isGoalReached_) {
			std::list<EnemyBase*> activeEnemies;
			for (const auto& enemy : enemies_) {
				activeEnemies.push_back(enemy.get());
			}
			player_->Update(activeEnemies);

			for (auto& enemy : enemies_) {
				enemy->Update();
			}

			CheckAllCollisions();

			for (auto it = enemies_.begin(); it != enemies_.end(); ) {
				if (!(*it)->IsAlive()) {
					if (dynamic_cast<RusherEnemy*>(it->get())) {
						auto shockwave = std::make_unique<Shockwave>();
						shockwave->Initialize(camera_.get(), (*it)->GetWorldPosition());
						shockwaves_.push_back(std::move(shockwave));
					}
					it = enemies_.erase(it);
				} else {
					++it;
				}
			}

			for (auto& shockwave : shockwaves_) {
				shockwave->Update();
			}
			shockwaves_.remove_if([](const std::unique_ptr<Shockwave>& shockwave) {
				return shockwave->IsFinished();
								  });

			if (lockOn_) {
				// LockOn::Update が求める「生ポインタのリスト」をその場で作成
				std::list<EnemyBase*> enemyPtrs;
				for (const auto& enemy : enemies_) {
					enemyPtrs.push_back(enemy.get());
				}

				// プレイヤー、作成した生ポインタリスト、カメラを渡して更新
				lockOn_->Update(player_.get(), enemyPtrs, camera_.get());
			}
		}

		// ------------------------------------
		// オブジェクト
		// ------------------------------------

		level_->Update();

		// ------------------------------------
		// 天球
		// ------------------------------------

		skydome_->Update();

		// ------------------------------------
		// パーティクルの更新
		// ------------------------------------
		ParticleManager::GetInstance()->Update(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());

		// HPが20以下の時に Vignetting 赤点滅を適用
		if (player_->GetHP() <= 20.0f) {
			PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kVignetting);

			// 点滅の計算 (サイン波を用いて明滅)
			static float vignetteTimer = 0.0f;
			vignetteTimer += 0.1f; // 点滅スピード

			float t = (sinf(vignetteTimer) + 1.0f) * 0.5f; // 0.0f 〜 1.0f のサイン波
			float red = 0.3f + t * 0.7f; // 最小0.3から最大1.0の赤さ
			PostProcessRenderer::GetInstance()->SetVignetteColor({red, 0.0f, 0.0f, 1.0f});
		} else {
			// HPが20より大きくなったら Vignetting モードを解除して通常状態にする
			if (PostProcessRenderer::GetInstance()->GetMode() == PostProcessRenderer::PostProcessMode::kVignetting) {
				PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kNormal);
			}
		}
	}
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
	// ゴール
	// ------------------------------------
	if (goal_) {
		goal_->Draw();
	}

	// ------------------------------------
	// 敵
	// ------------------------------------

	for (auto& enemy : enemies_) {
		enemy->Draw();
	}

	// ------------------------------------
	// 衝撃波エフェクト
	// ------------------------------------

	for (auto& shockwave : shockwaves_) {
		shockwave->Draw();
	}

	// ------------------------------------
	// 自機
	// ------------------------------------

	player_->Draw();

	// ------------------------------------
	// ロックオン
	// ------------------------------------

	lockOn_->Draw();

	// ------------------------------------
	// パーティクル描画
	// ------------------------------------
	ParticleManager::GetInstance()->Draw();
}


void GamePlayScene::Finalize() {}

void GamePlayScene::CheckAllCollisions() {
	// プレイヤーと敵の衝突判定
	for (auto& enemy : enemies_) {
		if (!enemy->IsAlive()) continue; // すでに死亡している敵はスキップ
		if (Collision::CheckCollision(player_.get(), enemy.get())) {
			player_->OnCollision();
			enemy->OnCollision();
			if (shake_) {
				shake_->Start(0.4f, 0.8f);
			}
		}
	}

	// プレイヤーの弾と敵の衝突判定
	const auto& bullets = player_->GetBullets();
	for (const auto& bullet : bullets) {
		if (bullet->IsDead()) continue; // すでに死亡している弾はスキップ
		for (auto& enemy : enemies_) {
			if (!enemy->IsAlive()) continue; // すでに死亡している敵はスキップ
			if (Collision::CheckCollision(bullet.get(), enemy.get())) {
				bullet->OnCollision();
				enemy->OnCollision();
			}
		}
	}
}

void GamePlayScene::UpdateImGui() {
#ifdef USE_IMGUI
	// 毎フレームエンティティを登録し直すことで、動的な増減に対応（ dangling pointer 防止 ）
	DevelopEditor* editor = DevelopEditor::GetInstance();
	editor->ClearEntities();
	editor->RegisterCamera("Main Camera", camera_.get(), [this]() {
		ImGui::Checkbox("Use Debug Camera", &useDebugCamera_);
	});
	if (railCamera_) {
		editor->RegisterEntity("Rail Camera", railCamera_->GetWorldTransform(), [this]() {
			bool isLoop = railCamera_->GetIsLoop();
			if (ImGui::Checkbox("Loop", &isLoop)) {
				railCamera_->SetIsLoop(isLoop);
			}
			float speed = railCamera_->GetSpeed();
			if (ImGui::DragFloat("Speed", &speed, 0.001f, 0.0f, 1.0f)) {
				railCamera_->SetSpeed(speed);
			}
		});

		editor->RegisterGameViewOverlay([this](ImDrawList* drawList, const Vector2& imageScreenPos, const Vector2& imageSize) {
			if (railCamera_) {
				railCamera_->DrawDebugSpline(drawList, imageScreenPos, imageSize);
			}
		});
	}
	if (skydome_) {
		editor->RegisterEntity("Skydome (天球)", &skydome_->GetWorldTransform());
	}
	if (player_) {
		editor->RegisterEntity("Player (自機)", player_->GetWorldTransform(), [this]() {
			float hp = player_->GetHP();
			if (ImGui::DragFloat("HP", &hp, 1.0f, 0.0f, 100.0f)) {
				player_->SetHP(hp);
			}
			ImGui::Text("Is LockOn Mode: %s", player_->GetIsLockOnMode() ? "True" : "False");
		});
	}
	if (goal_) {
		editor->RegisterEntity("Goal (ゴール)", &goal_->GetWorldTransform());
	}
	int enemyIdx = 0;
	for (auto& enemy : enemies_) {
		std::string name = std::format("Enemy_{}", enemyIdx++);
		editor->RegisterEntity(name, &enemy->GetWorldTransform(), [enemyPtr = enemy.get()]() {
			ImGui::Text("Status: %s", enemyPtr->IsAlive() ? "Alive" : "Dead");
		});
	}

	// 静的オブジェクトの登録
	int objIdx = 0;
	auto& levelObjs = level_->GetObjects();
	for (size_t i = 0; i < levelObjs.size(); ++i) {
		std::string name = std::format("Obj_{}", objIdx++);
		editor->RegisterEntity(name, levelObjs[i]->GetWorldTransform(), [this, i]() {
			if (ImGui::Button("Delete Object")) {
				DeleteObject(i);
			}
		});
	}

	// スプライトの登録
	int spriteIdx = 0;
	auto& levelSprites = level_->GetSprites();
	for (size_t i = 0; i < levelSprites.size(); ++i) {
		std::string name = std::format("Sprite_{}", spriteIdx++);
		editor->RegisterEntity(name, levelSprites[i]->GetWorldTransform(), [this, i]() {
			auto& levelSprites = level_->GetSprites();
			if (i < levelSprites.size()) {
				Vector4 color = levelSprites[i]->GetColor();
				if (ImGui::ColorEdit4("Color", &color.x)) {
					levelSprites[i]->SetColor(color);
				}
				if (ImGui::Button("Delete Sprite")) {
					DeleteSprite(i);
				}
			}
		});
	}

	// DevelopEditor全体の更新・描画
	editor->Update();
#endif
}

void GamePlayScene::PlaceNewObject(const std::string& assetName) {
	if (!levelData_) return;

	LevelData::ObjectData newObj;
	newObj.filename = assetName;
	
	Vector3 camPos = camera_->GetTranslate();
	Vector3 camRot = camera_->GetRotate();
	
	float cosPitch = cosf(camRot.x);
	Vector3 forward = {
		sinf(camRot.y) * cosPitch,
		-sinf(camRot.x),
		cosf(camRot.y) * cosPitch
	};
	newObj.translation = {
		camPos.x + forward.x * 20.0f,
		camPos.y + forward.y * 20.0f,
		camPos.z + forward.z * 20.0f
	};
	newObj.rotation = { 0, 0, 0 };
	newObj.scaling = { 1, 1, 1 };

	levelData_->objects.push_back(newObj);

	SaveLevel();
}

void GamePlayScene::DeleteObject(size_t index) {
	if (!levelData_) return;

	if (index < levelData_->objects.size()) {
		auto& levelObjs = level_->GetObjects();
		if (levelObjs.size() == levelData_->objects.size()) {
			for (size_t i = 0; i < levelObjs.size(); ++i) {
				levelData_->objects[i].translation = levelObjs[i]->GetPosition();
				levelData_->objects[i].rotation = levelObjs[i]->GetRotate();
				levelData_->objects[i].scaling = levelObjs[i]->GetScale();
			}
		}

		auto& levelSprites = level_->GetSprites();
		if (levelSprites.size() == levelData_->sprites.size()) {
			for (size_t i = 0; i < levelSprites.size(); ++i) {
				levelData_->sprites[i].translation = levelSprites[i]->GetPosition();
				levelData_->sprites[i].rotation = levelSprites[i]->GetRotate();
				levelData_->sprites[i].scaling = levelSprites[i]->GetScale();
				levelData_->sprites[i].color = levelSprites[i]->GetColor();
			}
		}

		levelData_->objects.erase(levelData_->objects.begin() + index);

		SaveLevel();
	}
}

void GamePlayScene::SaveLevel() {
	if (!levelData_) return;

	auto& levelObjs = level_->GetObjects();
	if (levelObjs.size() == levelData_->objects.size()) {
		for (size_t i = 0; i < levelObjs.size(); ++i) {
			levelData_->objects[i].translation = levelObjs[i]->GetPosition();
			levelData_->objects[i].rotation = levelObjs[i]->GetRotate();
			levelData_->objects[i].scaling = levelObjs[i]->GetScale();
		}
	}

	auto& levelSprites = level_->GetSprites();
	if (levelSprites.size() == levelData_->sprites.size()) {
		for (size_t i = 0; i < levelSprites.size(); ++i) {
			// 同期を強制的に行い最新情報を書き出す
			levelSprites[i]->Update();
			levelData_->sprites[i].translation = levelSprites[i]->GetPosition();
			levelData_->sprites[i].rotation = levelSprites[i]->GetRotate();
			levelData_->sprites[i].scaling = levelSprites[i]->GetScale();
			levelData_->sprites[i].color = levelSprites[i]->GetColor();
		}
	}

	LevelLoader loader;
	if (loader.Save(levelFilename_, levelData_.get())) {
		std::string fullpath = "resources/levels/" + levelFilename_ + ".json";
		if (std::filesystem::exists(fullpath)) {
			lastLevelWriteTime_ = std::filesystem::last_write_time(fullpath);
		}
		ReloadLevel();
	}
}

void GamePlayScene::ReloadLevel() {
	LevelLoader loader;
	levelData_ = loader.Load(levelFilename_);

	level_ = std::make_unique<Level>();
	level_->Initialize(levelData_.get(), camera_.get());

	level_->ApplyLightParameters();

	if (railCamera_) {
		railCamera_->SetControlPoints(levelData_->railSpline);
	}
}

void GamePlayScene::PlaceNewSprite(const std::string& assetName) {
	if (!levelData_) return;

	LevelData::SpriteData newSprite;
	newSprite.filename = assetName;
	newSprite.translation = { 640.0f, 360.0f };
	newSprite.rotation = 0.0f;
	newSprite.scaling = { 1.0f, 1.0f };
	newSprite.color = { 1.0f, 1.0f, 1.0f, 1.0f };

	levelData_->sprites.push_back(newSprite);

	SaveLevel();
}

void GamePlayScene::DeleteSprite(size_t index) {
	if (!levelData_) return;

	if (index < levelData_->sprites.size()) {
		auto& levelObjs = level_->GetObjects();
		if (levelObjs.size() == levelData_->objects.size()) {
			for (size_t i = 0; i < levelObjs.size(); ++i) {
				levelData_->objects[i].translation = levelObjs[i]->GetPosition();
				levelData_->objects[i].rotation = levelObjs[i]->GetRotate();
				levelData_->objects[i].scaling = levelObjs[i]->GetScale();
			}
		}

		auto& levelSprites = level_->GetSprites();
		if (levelSprites.size() == levelData_->sprites.size()) {
			for (size_t i = 0; i < levelSprites.size(); ++i) {
				levelData_->sprites[i].translation = levelSprites[i]->GetPosition();
				levelData_->sprites[i].rotation = levelSprites[i]->GetRotate();
				levelData_->sprites[i].scaling = levelSprites[i]->GetScale();
				levelData_->sprites[i].color = levelSprites[i]->GetColor();
			}
		}

		levelData_->sprites.erase(levelData_->sprites.begin() + index);

		SaveLevel();
	}
}