#include "GamePlayScene.h"

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
#include "FormationDroneEnemy.h"
#include "Shockwave.h"
#include "Collider.h"
#include "Shake.h"
#include "Goal.h"
#include "TimeManager.h"
#include "Sprite.h"
#include "CityBackground.h"
#include "WinApp.h"

using namespace MathUtility;

GamePlayScene::GamePlayScene() = default;
GamePlayScene::~GamePlayScene() = default;

void GamePlayScene::Initialize() {
	// カメラのインスタンス生成
	camera_ = std::make_unique<Camera>();

	// ------------------------------------
	// レベルデータのロード
	// ------------------------------------

	LevelLoader loader;
	std::unique_ptr<LevelData> levelData(loader.Load("formingEnemy"));

	// レベルオブジェクトの初期化
	level_ = std::make_unique<Level>();
	level_->Initialize(levelData.get(), camera_.get());

	// ------------------------------------
	// カメラ
	// ------------------------------------

	level_->ApplyCameraParameters(camera_.get());
	camera_->SetFarClip(600.0f); // 奥行きを出すためファークリップを大幅拡張 (100m -> 600m)
	camera_->CalculateMatrix();
	camera_->CreateConstantBuffer();

	// レールカメラ
	railCamera_ = std::make_unique<RailCameraController>();
	railCamera_->Initialize(camera_.get(), levelData->railSpline, "formingEnemy");

	// デバッグカメラ
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize();
	debugCamera_->SetRotate(camera_->GetRotate());
	debugCamera_->SetTranslate(camera_->GetTranslate());
	debugCamera_->SetFarClip(600.0f);
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

	// Spawnerデータから "Enemy" という名前が含まれるものをすべて取得して出現待ちリストに格納
	pendingEnemies_ = level_->GetSpawners("Enemy");

	// ------------------------------------
	// 背景 (黒スプライト & サイバー都市)
	// ------------------------------------

	backgroundSprite_ = std::make_unique<Sprite>();
	backgroundSprite_->Initialize("white.png", {0.0f, 0.0f}, {0.0f, 0.0f});
	backgroundSprite_->SetSize({static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight)});
	backgroundSprite_->SetColor({0.0f, 0.0f, 0.0f, 1.0f});

	cityBackground_ = std::make_unique<CityBackground>();
	if (railCamera_ && !railCamera_->GetControlPoints().empty()) {
		// コース（スプライン軌道）に沿ってゴール地点（＋奥の余白150m）までビル群を生成、80m〜300mで奥へ向かって徐々にフェードアウト
		cityBackground_->InitializeAlongPath(camera_.get(), railCamera_->GetControlPoints(), 10.0f, 8.0f, 150.0f, 80.0f, 300.0f);
	} else {
		cityBackground_->Initialize(camera_.get(), -20.0f, 320.0f, 65, 10.0f, 200.0f, 80.0f, 300.0f);
	}
	cityBackground_->SetScrollSpeed(0.0f); // レール移動するため静止配置

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
	if (railCamera_)
	{
		const auto& controlPoints = railCamera_->GetControlPoints();
		if (!controlPoints.empty())
		{
			goalPos = controlPoints.back();
		}
		// レールカメラのループをオフにしてゴール地点で停止するようにする
		railCamera_->SetIsLoop(false);
	}
	goal_->Initialize(goalPos, camera_.get());

	isGoalReached_ = false;
	clearWaitTimer_ = 0.0f;

	// パーティクルグループの作成と初期クリア
	ParticleManager::GetInstance()->CreateParticleGroup("CircleParticle", "resources/sprites/circle.png", ParticleManager::ParticleShape::kPlane);
	ParticleManager::GetInstance()->CreateParticleGroup("BulletTrail", "resources/sprites/circle.png", ParticleManager::ParticleShape::kPlane, ParticleManager::ShaderType::kBulletTrail);
	ParticleManager::GetInstance()->CreateParticleGroup("DigitalGlitchBox", "resources/sprites/white.png", ParticleManager::ParticleShape::kBox);
	ParticleManager::GetInstance()->ClearAllParticles();

	// ------------------------------------
	// UI
	// ------------------------------------

	uiPlayerHp_ = std::make_unique<Sprite>();
	uiPlayerHp_->Initialize("white.png", {10.0f, 10.0f}, {0.0f, 0.0f});
	uiPlayerHp_->SetSize({player_->GetHP() * 4.0f, 40.0f});

	// スコアの初期化
	score_ = 0;
	prevScore_ = -1;
	uiScoreDigits_.resize(kMaxScoreDigits);
	float startX = 1260.0f; // 右端の基準位置
	float startY = 20.0f;   // 上端の基準位置
	float digitWidth = 24.0f; // 数字の幅
	float digitHeight = 48.0f; // 数字の高さ
	for (int i = 0; i < kMaxScoreDigits; ++i)
	{
		uiScoreDigits_[i] = std::make_unique<Sprite>();
		// 右から左に向かって桁を並べる (1桁目は一番右)
		Vector2 pos = {startX - (i + 1) * digitWidth, startY};
		uiScoreDigits_[i]->Initialize("numbers/0.png", pos, {0.0f, 0.0f});
		uiScoreDigits_[i]->SetSize({digitWidth, digitHeight});
	}

	// ------------------------------------
	// フェードアウト用スプライト
	// ------------------------------------
	fadeSprite_ = std::make_unique<Sprite>();
	fadeSprite_->Initialize("white.png", {0.0f, 0.0f}, {0.0f, 0.0f});
	fadeSprite_->SetSize({static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight)});
	fadeSprite_->SetColor({0.0f, 0.0f, 0.0f, 0.0f});
	fadeAlpha_ = 0.0f;
	gameOverWaitTimer_ = 0.0f;

	phase_ = Phase::kPlay;

}

void GamePlayScene::Update() {
	// ------------------------------------
	// デバッグ・ショートカット: Tキーでタイトル画面へ遷移
	// ------------------------------------
	Input* input = Input::GetInstance();
	if (input->TriggerKey(DIK_T))
	{
		input->SetShake(0.0f, 0.0f);
		if (PostProcessRenderer::GetInstance()->GetMode() == PostProcessRenderer::PostProcessMode::kVignetting)
		{
			PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kNormal);
		}
		SceneManager::GetInstance()->ChangeScene("TITLE");
		return;
	}

	// ------------------------------------
	// ImGui
	// ------------------------------------
#ifdef USE_IMGUI
	UpdateImGui();
#endif


	// ------------------------------------
	// カメラ
	// ------------------------------------

	if (railCamera_)
	{
		// ゲームオーバー時はカメラワークを停止する
		if (phase_ != Phase::kGameOver)
		{
			railCamera_->Update(!useDebugCamera_);
		}
	}
	if (useDebugCamera_)
	{
		debugCamera_->Update(camera_.get());
	}

#ifdef USE_IMGUI
	if (railCamera_ && phase_ != Phase::kGameOver)
	{
		railCamera_->DrawDebugSpline();
	}
#endif

	// 画面シェイクの更新と適用
	if (shake_)
	{
		shake_->Update(TimeManager::GetInstance()->GetDeltaTime());
		// ゲームオーバー時はカメラの揺れも停止
		if (phase_ != Phase::kGameOver && shake_->IsActive() && !useDebugCamera_)
		{
			Vector3 offset = shake_->GetOffset();
			camera_->matWorld.m[3][0] += offset.x;
			camera_->matWorld.m[3][1] += offset.y;
			camera_->matWorld.m[3][2] += offset.z;

			camera_->matView = MathUtility::MakeInverseMatrix(camera_->matWorld);
			camera_->UpdateViewProjection();
		}
	}

	// プレイヤーが死亡している場合はゲームオーバーへ移行
	if (phase_ == Phase::kPlay && player_ && !player_->IsAlive())
	{
		ChangePhase(Phase::kGameOver);
	}

	if (phase_ == Phase::kPlay)
	{

		// ------------------------------------
		// ゴールの更新・アニメーション
		// ------------------------------------
		if (goal_)
		{
			goal_->Update();
		}

		// ------------------------------------
		// ゴール到達判定（衝突判定またはレールカメラ末尾到達）
		// ------------------------------------
		bool reachedGoal = false;
		// 自機との衝突判定によるゴール到達チェック
		if (player_ && goal_)
		{
			if (Collision::CheckCollision(player_.get(), goal_.get()))
			{
				reachedGoal = true;
			}
		}

		// カメラがレール末尾に到達したことによるゴール到達チェック
		if (railCamera_ && !railCamera_->GetIsLoop())
		{
			float maxTime = static_cast<float>((std::max) (0ULL, railCamera_->GetControlPoints().size()) - 1);
			if (railCamera_->GetSplineTime() >= maxTime)
			{
				reachedGoal = true;
			}
		}

		// ゴールに到達した場合はクリアフェーズへ移行（フェードアウト開始）
		if (reachedGoal)
		{
			ChangePhase(Phase::kClear);
			return;
		}

		// ------------------------------------
		// 自機 & 敵 & 衝突判定
		// ------------------------------------
		if (!isGoalReached_)
		{
			// 敵の発生タイミング判定 (タイムライン制御)
			if (railCamera_)
			{
				float currentSplineTime = railCamera_->GetSplineTime();
				for (auto it = pendingEnemies_.begin(); it != pendingEnemies_.end(); )
				{
					if (currentSplineTime >= it->spawnTime)
					{
						// entityType に応じて敵を分岐生成
						if (it->entityType.find("Drone") != std::string::npos ||
							it->entityType.find("Formation") != std::string::npos ||
							it->entityType.find("Wave") != std::string::npos)
						{
							DroneFlightPattern pattern = DroneFlightPattern::kSineWave;
							if (it->entityType.find("Circle") != std::string::npos)
							{
								pattern = DroneFlightPattern::kCircle;
							}
							else if (it->entityType.find("Slalom") != std::string::npos)
							{
								pattern = DroneFlightPattern::kSlalom;
							}
							else if (it->entityType.find("FigureEight") != std::string::npos)
							{
								pattern = DroneFlightPattern::kFigureEight;
							}

							SpawnDroneFormation(it->translation, pattern, 4);
						}
						else
						{
							auto enemy = std::make_unique<RusherEnemy>();
							enemy->Initialize(enemyModel_.get(), camera_.get(), it->translation, player_.get());
							enemy->GetWorldTransform().rotation = it->rotation;
							enemy->GetWorldTransform().scale = it->scaling;
							enemies_.push_back(std::move(enemy));
						}

						it = pendingEnemies_.erase(it);
					}
					else
					{
						++it;
					}
				}
			}

			std::list<EnemyBase*> activeEnemies;
			for (const auto& enemy : enemies_)
			{
				if (enemy->IsAlive())
				{
					activeEnemies.push_back(enemy.get());
				}
			}
			player_->Update(activeEnemies);

			for (auto& enemy : enemies_)
			{
				enemy->Update();
			}

			CheckAllCollisions();

			// 衝突判定によってプレイヤーが撃破された場合、直ちにゲームオーバーへ移行してフレームを終了
			if (!player_->IsAlive())
			{
				ChangePhase(Phase::kGameOver);
				return;
			}

			for (auto it = enemies_.begin(); it != enemies_.end(); )
			{
				// 撃破トリガー（演出開始時に1回だけ処理）
				if (!(*it)->IsAlive() && !(*it)->HasGivenScore())
				{
					(*it)->SetScoreGiven(true);

					// PlayerBulletに当たって死んだときのみスコア加算および撃破演出を実行
					if ((*it)->IsKilledByPlayerBullet())
					{
						// スコア加算
						score_ += (*it)->GetScore();

						// 敵撃破時の衝撃波エフェクト生成
						auto shockwave = std::make_unique<Shockwave>();
						shockwave->Initialize(camera_.get(), (*it)->GetWorldPosition());
						shockwaves_.push_back(std::move(shockwave));

						// 撃破時のマイクロシェイク
						if (shake_)
						{
							shake_->Start(0.15f, 0.35f);
						}
					}
				}

				// 死亡演出が完全に終わった敵を削除
				if ((*it)->IsDead())
				{
					it = enemies_.erase(it);
				}
				else
				{
					++it;
				}
			}

			for (auto& shockwave : shockwaves_)
			{
				shockwave->Update();
			}
			shockwaves_.remove_if([](const std::unique_ptr<Shockwave>& shockwave) {
				return shockwave->IsFinished();
								  });

			if (lockOn_)
			{
				// LockOn::Update が求める「生ポインタのリスト」をその場で作成
				std::list<EnemyBase*> enemyPtrs;
				for (const auto& enemy : enemies_)
				{
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
		// 背景更新
		// ------------------------------------

		if (backgroundSprite_) {
			backgroundSprite_->Update();
		}
		if (cityBackground_) {
			cityBackground_->SetCamera(camera_.get());
			cityBackground_->Update();
		}

		// ------------------------------------
		// パーティクルの更新
		// ------------------------------------
		ParticleManager::GetInstance()->Update(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());

		// 生存中かつHPが20以下の時に Vignetting 赤点滅を適用
		if (player_->IsAlive() && player_->GetHP() <= 20.0f)
		{
			PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kVignetting);

			// 点滅の計算 (サイン波を用いて明滅)
			static float vignetteTimer = 0.0f;
			vignetteTimer += 0.1f; // 点滅スピード

			float t = (sinf(vignetteTimer) + 1.0f) * 0.5f; // 0.0f 〜 1.0f のサイン波
			float red = 0.3f + t * 0.7f; // 最小0.3から最大1.0の赤さ
			PostProcessRenderer::GetInstance()->SetVignetteColor({red, 0.0f, 0.0f, 1.0f});
		} else
		{
			// HPが20より大きくなった、または死亡時は Vignetting モードを解除して通常状態にする
			if (PostProcessRenderer::GetInstance()->GetMode() == PostProcessRenderer::PostProcessMode::kVignetting)
			{
				PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kNormal);
			}
		}
	}
	else if (phase_ == Phase::kClear)
	{
		// ------------------------------------
		// クリアフェーズ（ゴール到達・フェードアウト演出）
		// レールカメラと敵の動きは停止
		// ゴールや自機のアニメーション、背景、パーティクルは継続
		// ------------------------------------

		// ヴィネットが残っていれば確実に消去
		if (PostProcessRenderer::GetInstance()->GetMode() == PostProcessRenderer::PostProcessMode::kVignetting)
		{
			PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kNormal);
		}

		float dt = TimeManager::GetInstance()->GetDeltaTime();

		// 1. ゴールオブジェクトの更新（回転・UVアニメーション継続）
		if (goal_)
		{
			goal_->Update();
		}

		// 2. 自機の更新（操作や弾撃ちは行わず、アニメーションとトランスフォームのみ更新）
		if (player_)
		{
			player_->UpdateAnimationOnly();
		}

		// 3. 背景都市・オブジェクト・パーティクルの更新
		level_->Update();
		if (backgroundSprite_)
		{
			backgroundSprite_->Update();
		}
		if (cityBackground_)
		{
			cityBackground_->SetCamera(camera_.get());
			cityBackground_->Update();
		}
		ParticleManager::GetInstance()->Update(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());

		// 4. フェードアウトの進行
		fadeAlpha_ += dt / kClearFadeDuration;
		if (fadeAlpha_ >= 1.0f)
		{
			fadeAlpha_ = 1.0f;
			clearWaitTimer_ += dt;
		}

		if (fadeSprite_)
		{
			fadeSprite_->SetColor({0.0f, 0.0f, 0.0f, fadeAlpha_});
			fadeSprite_->Update();
		}

		// 5. フェードアウト完了後、余韻待機時間経過（またはENTER / Aボタン入力）でタイトルシーンへ遷移
		if (fadeAlpha_ >= 1.0f)
		{
			if (input->TriggerKey(DIK_RETURN) || input->TriggerButton(XINPUT_GAMEPAD_A) || clearWaitTimer_ >= kClearWaitDuration)
			{
				PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kNormal);
				SceneManager::GetInstance()->ChangeScene("TITLE");
				return;
			}
		}
		else
		{
			// フェードアウト中もENTER / Aボタンで即座にタイトルへ戻れるようにする
			if (input->TriggerKey(DIK_RETURN) || input->TriggerButton(XINPUT_GAMEPAD_A))
			{
				PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kNormal);
				SceneManager::GetInstance()->ChangeScene("TITLE");
				return;
			}
		}
	}
	else if (phase_ == Phase::kGameOver)
	{
		// ------------------------------------
		// ゲームオーバーフェーズ
		// カメラワークと敵の動きは停止（Updateを呼ばない）
		// ヴィネットは即座に消去され、プレイヤーの破壊演出（ディゾルブ）が進行
		// 破壊演出が完了した後にフェードアウトを開始する
		// ------------------------------------

		// ヴィネットが残っていれば確実に消去
		if (PostProcessRenderer::GetInstance()->GetMode() == PostProcessRenderer::PostProcessMode::kVignetting)
		{
			PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kNormal);
		}

		// 1. プレイヤーの更新（ディゾルブ消滅・破壊演出を進行させる）
		std::list<EnemyBase*> emptyEnemies;
		player_->Update(emptyEnemies);

		float dt = TimeManager::GetInstance()->GetDeltaTime();

		// 2. プレイヤーの破壊演出（ディゾルブ）が完了してからフェードアウトを開始
		if (player_->GetDissolveThreshold() >= 1.0f)
		{
			fadeAlpha_ += dt / kGameOverFadeDuration;
			if (fadeAlpha_ >= 1.0f)
			{
				fadeAlpha_ = 1.0f;
				gameOverWaitTimer_ += dt;
			}
		}

		if (fadeSprite_)
		{
			fadeSprite_->SetColor({0.0f, 0.0f, 0.0f, fadeAlpha_});
			fadeSprite_->Update();
		}

		// 3. フェードアウト完了後、ENTERキー/Aボタン、もしくはフェードアウト完了後2秒経過でタイトルへ戻る
		if (fadeAlpha_ >= 1.0f)
		{
			if (input->TriggerKey(DIK_RETURN) || input->TriggerButton(XINPUT_GAMEPAD_A) || gameOverWaitTimer_ >= 2.0f)
			{
				PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kNormal);
				SceneManager::GetInstance()->ChangeScene("TITLE");
				return;
			}
		}
	}

	// ------------------------------------
	// UI
	// ------------------------------------

	uiPlayerHp_->SetSize({player_->GetHP() * 4.0f, 40.0f});

	uiPlayerHp_->Update();

	// スコアの更新があった場合のみテクスチャを再設定
	if (score_ != prevScore_)
	{
		int temp = score_;
		for (int i = 0; i < kMaxScoreDigits; ++i)
		{
			int digit = temp % 10;
			temp /= 10;
			std::string path = "numbers/" + std::to_string(digit) + ".png";
			uiScoreDigits_[i]->SetTexture(path);
		}
		prevScore_ = score_;
	}

	// スコアスプライトの更新
	for (auto& digitSprite : uiScoreDigits_)
	{
		digitSprite->Update();
	}
}

void GamePlayScene::Draw() {
	// ------------------------------------
	// 背景 (最背面の黒スプライト & サイバー都市)
	// ------------------------------------

	if (backgroundSprite_) {
		backgroundSprite_->Draw();
	}
	if (cityBackground_) {
		cityBackground_->Draw();
	}

	// ------------------------------------
	// オブジェクト
	// ------------------------------------

	level_->Draw();

	// ------------------------------------
	// ゴール
	// ------------------------------------
	if (goal_)
	{
		goal_->Draw();
	}

	// ------------------------------------
	// 敵
	// ------------------------------------

	for (auto& enemy : enemies_)
	{
		enemy->Draw();
	}

	// ------------------------------------
	// 衝撃波エフェクト
	// ------------------------------------

	for (auto& shockwave : shockwaves_)
	{
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

	// ------------------------------------
	// UI
	// ------------------------------------

	uiPlayerHp_->Draw();

	// スコアUIの描画
	for (auto& digitSprite : uiScoreDigits_)
	{
		digitSprite->Draw();
	}

	// ------------------------------------
	// フェードアウト (最前面に描画)
	// ------------------------------------
	if (fadeSprite_ && fadeAlpha_ > 0.0f)
	{
		fadeSprite_->Draw();
	}
}


void GamePlayScene::Finalize() {
	backgroundSprite_.reset();
	cityBackground_.reset();
	fadeSprite_.reset();

	// シーン終了時にポストプロセスを通常状態に戻す
	if (PostProcessRenderer::GetInstance()->GetMode() == PostProcessRenderer::PostProcessMode::kVignetting)
	{
		PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kNormal);
	}
}

void GamePlayScene::CheckAllCollisions() {
	// プレイヤーと敵の衝突判定
	for (auto& enemy : enemies_)
	{
		if (!enemy->IsAlive()) continue; // すでに死亡している敵はスキップ
		if (Collision::CheckCollision(player_.get(), enemy.get()))
		{
			player_->OnCollision();
			enemy->OnCollision();
			if (shake_)
			{
				shake_->Start(0.4f, 0.8f);
			}
		}
	}

	// プレイヤーの弾と敵の衝突判定
	const auto& bullets = player_->GetBullets();
	for (const auto& bullet : bullets)
	{
		if (bullet->IsDead()) continue; // すでに死亡している弾はスキップ
		for (auto& enemy : enemies_)
		{
			if (!enemy->IsAlive()) continue; // すでに死亡している敵はスキップ
			if (Collision::CheckCollision(bullet, enemy.get()))
			{
				bullet->OnCollision();
				enemy->OnCollision();
				enemy->SetKilledByPlayerBullet(true);
			}
		}
	}
}

void GamePlayScene::ChangePhase(Phase nextPhase) {
	// 次のフェーズをセット
	phase_ = nextPhase;

	switch (phase_)
	{
	case Phase::kLeady:
		break;
	case Phase::kPlay:
		break;
	case Phase::kClear:
		isGoalReached_ = true;
		// 1. レールカメラの動きを停止
		if (railCamera_)
		{
			railCamera_->SetIsPlaying(false);
		}
		// 2. ヴィネットを確実に消去して通常描画に戻す
		PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kNormal);
		// 3. フェードアウトの初期化
		fadeAlpha_ = 0.0f;
		clearWaitTimer_ = 0.0f;
		if (fadeSprite_)
		{
			fadeSprite_->SetColor({0.0f, 0.0f, 0.0f, 0.0f});
			fadeSprite_->Update();
		}
		break;
	case Phase::kGameOver:
		// 1. レールカメラの動きを停止
		if (railCamera_)
		{
			railCamera_->SetIsPlaying(false);
		}
		// 2. ヴィネットを確実に消去して通常描画に戻す
		PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kNormal);
		// 3. フェードアウトの初期化
		fadeAlpha_ = 0.0f;
		gameOverWaitTimer_ = 0.0f;
		if (fadeSprite_)
		{
			fadeSprite_->SetColor({0.0f, 0.0f, 0.0f, 0.0f});
			fadeSprite_->Update();
		}
		break;
	}
}

void GamePlayScene::SpawnDroneFormation(const Vector3& basePos, DroneFlightPattern pattern, int count) {
	for (int i = 0; i < count; ++i)
	{
		FormationDroneEnemy::FormationConfig config;
		config.pattern = pattern;
		config.phaseOffset = static_cast<float>(i) * 0.45f;

		if (pattern == DroneFlightPattern::kSineWave)
		{
			config.localOffset = { (i - (count - 1) * 0.5f) * 2.0f, 0.0f, static_cast<float>(i) * 3.0f };
			config.speed = 18.0f;
			config.waveAmplitude = 4.0f;
			config.waveFrequency = 2.0f;
		}
		else if (pattern == DroneFlightPattern::kCircle)
		{
			config.localOffset = { 0.0f, 0.0f, static_cast<float>(i) * 2.5f };
			config.speed = 14.0f;
			config.waveAmplitude = 5.0f;
			config.waveFrequency = 2.5f;
		}
		else if (pattern == DroneFlightPattern::kSlalom)
		{
			config.localOffset = { 0.0f, (i - (count - 1) * 0.5f) * 0.8f, static_cast<float>(i) * 3.5f };
			config.speed = 20.0f;
			config.waveAmplitude = 6.0f;
			config.waveFrequency = 2.0f;
		}
		else if (pattern == DroneFlightPattern::kFigureEight)
		{
			config.localOffset = { 0.0f, 0.0f, static_cast<float>(i) * 3.0f };
			config.speed = 15.0f;
			config.waveAmplitude = 5.0f;
			config.waveFrequency = 2.0f;
		}

		auto drone = std::make_unique<FormationDroneEnemy>();
		drone->Initialize(enemyModel_.get(), camera_.get(), basePos, config);
		enemies_.push_back(std::move(drone));
	}
}

void GamePlayScene::UpdateImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("デバッグウィンドウ");

	// FPSを表示
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Text("Score: %d", score_);
	ImGui::Text("Active Enemies: %d", static_cast<int>(enemies_.size()));

	const char* phaseStr = "Unknown";
	switch (phase_) {
	case Phase::kLeady: phaseStr = "Ready"; break;
	case Phase::kPlay: phaseStr = "Play"; break;
	case Phase::kClear: phaseStr = "Clear"; break;
	case Phase::kGameOver: phaseStr = "GameOver"; break;
	}
	ImGui::Text("Phase: %s", phaseStr);
	ImGui::Text("Fade Alpha: %.2f", fadeAlpha_);
	if (phase_ == Phase::kPlay) {
		if (ImGui::Button("Debug: Trigger Goal (Clear)")) {
			ChangePhase(Phase::kClear);
		}
	}

	if (ImGui::CollapsingHeader("City Background", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (cityBackground_)
		{
			float baseY = cityBackground_->GetBaseY();
			if (ImGui::SliderFloat("Base Y (Height)", &baseY, -50.0f, 10.0f, "%.1f"))
			{
				cityBackground_->SetBaseY(baseY);
			}

			float fogNear = cityBackground_->GetFogNear();
			float fogFar = cityBackground_->GetFogFar();
			bool fogChanged = false;
			if (ImGui::SliderFloat("Depth Fade Near (Start)", &fogNear, 10.0f, 300.0f, "%.1f m")) {
				fogChanged = true;
			}
			if (ImGui::SliderFloat("Depth Fade Far (End)", &fogFar, 50.0f, 600.0f, "%.1f m")) {
				fogChanged = true;
			}
			if (fogChanged) {
				cityBackground_->SetFog(fogNear, fogFar);
			}

			if (camera_) {
				float farClip = camera_->GetFarClip();
				if (ImGui::SliderFloat("Camera Far Clip", &farClip, 100.0f, 1000.0f, "%.0f m")) {
					camera_->SetFarClip(farClip);
					if (debugCamera_) {
						debugCamera_->SetFarClip(farClip);
					}
				}
			}
		}
	}

	if (ImGui::CollapsingHeader("Enemy Spawner Debug"))
	{
		Vector3 spawnFront = { 0.0f, 0.0f, 60.0f };
		if (player_)
		{
			spawnFront = player_->GetWorldTransform()->GetWorldPosition() + Vector3{ 0.0f, 0.0f, 80.0f };
		}

		if (ImGui::Button("Spawn Wave Formation (4)"))
		{
			SpawnDroneFormation(spawnFront, DroneFlightPattern::kSineWave, 4);
		}
		ImGui::SameLine();
		if (ImGui::Button("Spawn Circle Formation (4)"))
		{
			SpawnDroneFormation(spawnFront, DroneFlightPattern::kCircle, 4);
		}
		if (ImGui::Button("Spawn Slalom Formation (4)"))
		{
			SpawnDroneFormation(spawnFront, DroneFlightPattern::kSlalom, 4);
		}
		ImGui::SameLine();
		if (ImGui::Button("Spawn 8-Shape Formation (4)"))
		{
			SpawnDroneFormation(spawnFront, DroneFlightPattern::kFigureEight, 4);
		}
	}

	ImGui::End();
#endif
}