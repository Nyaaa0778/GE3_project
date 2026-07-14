#include "GamePlayScene.h"
#include "DevEditor.h"

#include <MyEngine.h>
#include "PostProcessRenderer.h"
#include "TextureManager.h"
#include "Input.h"

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
	railCamera_->Initialize(camera_.get(), levelData->railSpline, "TL1Sample");

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
	Vector3 goalPos = { 0.0f, 0.0f, 150.0f };
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
	// ------------------------------------
	// ImGui
	// ------------------------------------
#ifdef USE_IMGUI
	UpdateImGui();

	bool isPaused = DevEditor::GetInstance()->IsPaused();
	if (DevEditor::GetInstance()->IsStepRequested()) {
		isPaused = false;
		DevEditor::GetInstance()->ClearStepRequest();
	}
#else
	bool isPaused = false;
#endif

	// ------------------------------------
	// カメラ
	// ------------------------------------

	if (railCamera_) {
		railCamera_->Update(!useDebugCamera_ && !isPaused);
	}
	if (useDebugCamera_) {
		debugCamera_->Update(camera_.get());
	}

#ifdef USE_IMGUI
	if (railCamera_ && !DevEditor::GetInstance()->IsEditorMode()) {
		railCamera_->DrawDebugSpline();
	}
#endif

	// 画面シェイクの更新と適用（一時停止中でない場合、またはデバッグカメラ不使用時のみ）
	if (shake_) {
		if (!isPaused) {
			shake_->Update(TimeManager::GetInstance()->GetDeltaTime());
		}
		if (shake_->IsActive() && !useDebugCamera_) {
			Vector3 offset = shake_->GetOffset();
			camera_->matWorld.m[3][0] += offset.x;
			camera_->matWorld.m[3][1] += offset.y;
			camera_->matWorld.m[3][2] += offset.z;

			camera_->matView = MathUtility::MakeInverseMatrix(camera_->matWorld);
			camera_->UpdateViewProjection();
		}
	}

	// 一時停止中でない場合のみゲームプレイロジックを更新
	if (!isPaused) {
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
				float maxTime = static_cast<float>((std::max)(0ULL, railCamera_->GetControlPoints().size()) - 1);
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
				std::list<EnemyBase*> enemyPtrs;
				for (const auto& enemy : enemies_) {
					enemyPtrs.push_back(enemy.get());
				}
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
	}

	// ------------------------------------
	// パーティクルの更新 (一時停止中は更新しない)
	// ------------------------------------
	if (!isPaused) {
		ParticleManager::GetInstance()->Update(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());
	}

	// HPが20以下の時に Vignetting 赤点滅を適用
	if (player_->GetHP() <= 20.0f) {
		PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kVignetting);

		static float vignetteTimer = 0.0f;
		if (!isPaused) {
			vignetteTimer += 0.1f; // 点滅スピード
		}
		
		float t = (sinf(vignetteTimer) + 1.0f) * 0.5f;
		float red = 0.3f + t * 0.7f;
		PostProcessRenderer::GetInstance()->SetVignetteColor({ red, 0.0f, 0.0f, 1.0f });
	}
	else {
		if (PostProcessRenderer::GetInstance()->GetMode() == PostProcessRenderer::PostProcessMode::kVignetting) {
			PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kNormal);
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
	bool isEditor = DevEditor::GetInstance()->IsEditorMode();
	if (isEditor) {
		if (isGoalReached_) {
			DrawGoalReachedOverlay();
		}
		return;
	}

	if (ImGui::Begin("ウィンドウ")) {
		DrawSceneInspector();
	}
	ImGui::End(); 

	if (goal_) {
		goal_->DrawImGui("ゴール設定");
	}

	if (isGoalReached_) {
		DrawGoalReachedOverlay();
	}
#endif
}

void GamePlayScene::DrawSceneInspector() {
#ifdef USE_IMGUI
	// ─────────────────────
	// Player Object
	// ─────────────────────

	ImGui::SeparatorText("プレイヤー");

	{
		float playerHp = player_->GetHP();
		if (ImGui::SliderFloat("HP", &playerHp, 0.0f, 100.0f, "%.1f")) {
			player_->SetHP(playerHp);
		}
	}

	{
		Vector3 pos = player_->GetWorldTransform()->translation;
		if (ImGui::DragFloat3("座標", &pos.x, 0.1f)) {
			player_->GetWorldTransform()->translation = pos;
		}
	}

	{
		Vector3 scale = player_->GetWorldTransform()->scale;
		if (ImGui::DragFloat3("スケール", &scale.x, 0.1f, -10.0f, 10.0f)) {
			player_->GetWorldTransform()->scale = scale;
		}
	}

	{
		Vector3 rot = player_->GetWorldTransform()->rotation;
		if (ImGui::DragFloat3("回転", &rot.x, 0.1f, -6.28f, 6.28f)) {
			player_->GetWorldTransform()->rotation = rot;
		}
	}

	{
		Vector4 color = playerModel_->GetColor();
		float col[4] = {color.x, color.y, color.z, color.w};

		// ImGui カラーピッカー
		if (ImGui::ColorEdit4("色", col)) {
			Vector4 newColor(col[0], col[1], col[2], col[3]);
			playerModel_->SetColor(newColor);
		}
	}

	// ─────────────────────
	// カメラ
	// ─────────────────────

	ImGui::SeparatorText("カメラ");

	if (ImGui::Checkbox("デバッグカメラを使用", &useDebugCamera_)) {
		if (useDebugCamera_) {
			debugCamera_->SetRotate(camera_->GetRotate());
			debugCamera_->SetTranslate(camera_->GetTranslate());
			debugCamera_->CalculateMatrix();
		}
	}

	// 位置
	{
		Vector3 pos = camera_->GetTranslate();
		if (ImGui::DragFloat3("カメラの座標", &pos.x, 0.1f)) {
			camera_->SetTranslate(pos);
		}
	}

	// 回転
	{
		Vector3 rot = camera_->GetRotate();
		if (ImGui::DragFloat3("カメラの回転", &rot.x, 0.01f)) {
			camera_->SetRotate(rot);
		}
	}

	// ─────────────────────
	// PostProcess
	// ─────────────────────
	ImGui::SeparatorText("ポストプロセス設定");
	{
		static int currentMode = static_cast<int>(PostProcessRenderer::GetInstance()->GetMode());
		const char* modes[] = {"通常", "ラジアルブラー", "ボックスフィルタ", "ガウシアンフィルタ", "グレースケール", "アウトライン", "ビネット", "ディゾルブ"};
		if (ImGui::Combo("描画モード", &currentMode, modes, IM_ARRAYSIZE(modes))) {
			PostProcessRenderer::GetInstance()->SetMode(static_cast<PostProcessRenderer::PostProcessMode>(currentMode));
		}

		if (currentMode == static_cast<int>(PostProcessRenderer::PostProcessMode::kVignetting)) {
			Vector4 color = PostProcessRenderer::GetInstance()->GetVignetteColor();
			float c[4] = {color.x, color.y, color.z, color.w};
			if (ImGui::ColorEdit4("ビネットカラー", c)) {
				PostProcessRenderer::GetInstance()->SetVignetteColor({c[0], c[1], c[2], c[3]});
			}
		}

		if (currentMode == static_cast<int>(PostProcessRenderer::PostProcessMode::kDissolve)) {
			float threshold = PostProcessRenderer::GetInstance()->GetDissolveThreshold();
			if (ImGui::SliderFloat("ディゾルブしきい値", &threshold, 0.0f, 1.0f)) {
				PostProcessRenderer::GetInstance()->SetDissolveThreshold(threshold);
			}

			static int noiseIndex = 0;
			const char* noises[] = {"ノイズ0", "ノイズ1"};
			if (ImGui::Combo("ディゾルブノイズ", &noiseIndex, noises, IM_ARRAYSIZE(noises))) {
				if (noiseIndex == 0) {
					PostProcessRenderer::GetInstance()->SetDissolveNoiseTexture("resources/sprites/noise0.png");
				} else {
					PostProcessRenderer::GetInstance()->SetDissolveNoiseTexture("resources/sprites/noise1.png");
				}
			}
		}
	}
#endif
}

void GamePlayScene::DrawGoalReachedOverlay() {
#ifdef USE_IMGUI
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(450.0f, 220.0f));
	ImGuiWindowFlags clearFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 0.85f)); // Dark glassmorphism-like background
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

	if (ImGui::Begin("Game Clear Overlay", nullptr, clearFlags)) {
		ImGui::Spacing();
		ImGui::Spacing();

		// Gold colored text
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
		ImGui::SetWindowFontScale(2.5f);
		const char* titleText = "STAGE CLEAR!";
		float textWidth = ImGui::CalcTextSize(titleText).x * 2.5f;
		ImGui::SetCursorPosX((450.0f - textWidth) * 0.5f);
		ImGui::Text("%s", titleText);
		ImGui::PopStyleColor();
		ImGui::SetWindowFontScale(1.0f);

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		const char* subText = "ENTERキーまたは [A]ボタン を押してタイトルに戻る";
		float subTextWidth = ImGui::CalcTextSize(subText).x;
		ImGui::SetCursorPosX((450.0f - subTextWidth) * 0.5f);
		ImGui::Text("%s", subText);

		ImGui::Spacing();
		ImGui::Spacing();

		// Center button
		float btnWidth = 150.0f;
		ImGui::SetCursorPosX((450.0f - btnWidth) * 0.5f);
		if (ImGui::Button("タイトルに戻る", ImVec2(btnWidth, 35.0f))) {
			SceneManager::GetInstance()->ChangeScene("TITLE");
		}
	}
	ImGui::End(); 

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor();
#endif
}

void GamePlayScene::DrawEditorHierarchyAndInspector() {
#ifdef USE_IMGUI
	// DevEditorのヒエラルキーとインスペクター登録
	DevEditor::GetInstance()->DrawHierarchy("ゲームプレイシーン ヒエラルキー", [this]() {
		
		// Gameplay Stage Scene Settings
		if (DevEditor::GetInstance()->HierarchyNode("ゲームプレイステージ (グローバル)", this)) {
			DevEditor::GetInstance()->SetInspectorDrawer([this]() {
				DrawSceneInspector();
			});
		}
		// 1. プレイヤーオブジェクト
		if (player_) {
			if (DevEditor::GetInstance()->HierarchyNode("プレイヤー", player_.get())) {
				DevEditor::GetInstance()->SetInspectorDrawer([this]() {
					float playerHp = player_->GetHP();
					if (ImGui::SliderFloat("プレイヤー HP", &playerHp, 0.0f, 100.0f, "%.1f")) {
						player_->SetHP(playerHp);
					}
					
					DevEditor::GetInstance()->DrawTransformEdit(player_->GetWorldTransform());
					
					Vector4 color = playerModel_->GetColor();
					float col[4] = {color.x, color.y, color.z, color.w};
					if (ImGui::ColorEdit4("モデルの色", col)) {
						playerModel_->SetColor({col[0], col[1], col[2], col[3]});
					}
				});
			}
		}

		// 2. カメラオブジェクト
		if (camera_) {
			if (DevEditor::GetInstance()->HierarchyNode("メインカメラ", camera_.get())) {
				DevEditor::GetInstance()->SetInspectorDrawer([this]() {
					ImGui::Checkbox("デバッグカメラを使用", &useDebugCamera_);
					ImGui::Separator();
					
					Vector3 pos = camera_->GetTranslate();
					if (ImGui::DragFloat3("座標", &pos.x, 0.1f)) {
						camera_->SetTranslate(pos);
						camera_->CalculateMatrix();
					}
					
					Vector3 rot = camera_->GetRotate();
					if (ImGui::DragFloat3("回転", &rot.x, 0.01f)) {
						camera_->SetRotate(rot);
						camera_->CalculateMatrix();
					}
				});
			}
		}

		// 3. レールカメラ
		if (railCamera_) {
			if (DevEditor::GetInstance()->HierarchyNode("レールカメラ", railCamera_.get())) {
				DevEditor::GetInstance()->SetInspectorDrawer([this]() {
					railCamera_->DrawInspector();
				});
			}
		}

		// 4. ゴール
		if (goal_) {
			if (DevEditor::GetInstance()->HierarchyNode("ゴールターゲット", goal_.get())) {
				DevEditor::GetInstance()->SetInspectorDrawer([this]() {
					DevEditor::GetInstance()->DrawTransformEdit(&goal_->GetWorldTransform());
				});
			}
		}

		// 4. 敵の一覧
		int enemyIdx = 0;
		for (auto& enemy : enemies_) {
			std::string name = "敵_" + std::to_string(enemyIdx++);
			if (DevEditor::GetInstance()->HierarchyNode(name.c_str(), enemy.get())) {
				DevEditor::GetInstance()->SetInspectorDrawer([enemy = enemy.get()]() {
					DevEditor::GetInstance()->DrawTransformEdit(&enemy->GetWorldTransform());
				});
			}
		}

		// 5. ポストプロセス管理
		static int postProcessNodeId = 9999;
		if (DevEditor::GetInstance()->HierarchyNode("ポストプロセス設定", &postProcessNodeId)) {
			DevEditor::GetInstance()->SetInspectorDrawer([this]() {
				int currentMode = static_cast<int>(PostProcessRenderer::GetInstance()->GetMode());
				const char* modes[] = {"通常", "ラジアルブラー", "ボックスフィルタ", "ガウシアンフィルタ", "グレースケール", "アウトライン", "ビネット", "ディゾルブ"};
				if (ImGui::Combo("描画モード", &currentMode, modes, IM_ARRAYSIZE(modes))) {
					PostProcessRenderer::GetInstance()->SetMode(static_cast<PostProcessRenderer::PostProcessMode>(currentMode));
				}

				if (currentMode == static_cast<int>(PostProcessRenderer::PostProcessMode::kVignetting)) {
					Vector4 color = PostProcessRenderer::GetInstance()->GetVignetteColor();
					float c[4] = {color.x, color.y, color.z, color.w};
					if (ImGui::ColorEdit4("ビネットカラー", c)) {
						PostProcessRenderer::GetInstance()->SetVignetteColor({c[0], c[1], c[2], c[3]});
					}
				}

				if (currentMode == static_cast<int>(PostProcessRenderer::PostProcessMode::kDissolve)) {
					float threshold = PostProcessRenderer::GetInstance()->GetDissolveThreshold();
					if (ImGui::SliderFloat("ディゾルブしきい値", &threshold, 0.0f, 1.0f)) {
						PostProcessRenderer::GetInstance()->SetDissolveThreshold(threshold);
					}
				}
			});
		}
	});
#endif
}

void GamePlayScene::DrawEditorOverlay() {
#ifdef USE_IMGUI
	if (railCamera_) {
		railCamera_->DrawDebugSpline();
	}
#endif
}