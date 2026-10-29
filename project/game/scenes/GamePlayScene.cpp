#include "GamePlayScene.h"

#include <MyEngine.h>
#include "PostProcessRenderer.h"
#include "PostProcessEffects.h"
#include "CrtTransition.h"
#include "GameOverUI.h"
#include "TextureManager.h"
#include "Input.h"

#include "LevelLoader.h"
#include "Level.h"

#include "MathUtility.h"
#include "LightManager.h"

#include "LockOn.h"
#include "Player.h"
#include "Reticle.h"
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

	// ------------------------------------
	// ゲームオーバー演出UI
	// ------------------------------------
	gameOverUI_ = std::make_unique<GameOverUI>();
	gameOverUI_->Initialize();
	isPlayerDestroyEffectTriggered_ = false;
	isOuterBreakTriggered_ = false;
	isMiddleBreakTriggered_ = false;
	isCoreBreakTriggered_ = false;

	// 画面中央上部の自死案内スプライトUI（[K] キーで自滅）の初期化
	uiKillHint_ = std::make_unique<Sprite>();
	uiKillHint_->Initialize("kill_hint.png", { 640.0f, 55.0f }, { 0.5f, 0.5f });
	uiKillHint_->SetSize({ 460.0f, 62.0f });

	// ------------------------------------
	// ステージ開始演出の一時的無効化（即座にゲームプレイ開始）
	// 元に戻す場合は ChangePhase(Phase::kPlay); をコメントアウトし、
	// 下記の kLeady 処理のコメントを解除してください
	// ------------------------------------
	ChangePhase(Phase::kPlay);

	/*
	introCameraTimer_ = 0.0f;
	playerMaterializeTimer_ = 0.0f;
	if (railCamera_)
	{
		railCamera_->SetIsPlaying(false);
		railCamera_->SetSplineTime(0.0f);
	}
	if (player_)
	{
		player_->SetMaterializeProgress(0.0f);
	}
	playerMaterializedEffectTriggered_ = false;
	phase_ = Phase::kLeady;
	UpdateIntroCamera();
	*/
}

void GamePlayScene::Update() {
	// ------------------------------------
	// デバッグ・ショートカット: Tキーでタイトル画面へ遷移（CRT画面遷移演出）
	// ------------------------------------
	Input* input = Input::GetInstance();

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
		// イントロ中やゲームオーバー時はレールカメラの自動進行を停止
		if (phase_ != Phase::kGameOver && phase_ != Phase::kLeady)
		{
			railCamera_->Update(!useDebugCamera_);
		}
		else if (phase_ == Phase::kGameOver && !useDebugCamera_)
		{
			// ゲームオーバー時は進行は停止するが、カメラの基準姿勢を同期して位置を固定
			camera_->matWorld = railCamera_->GetWorldTransform()->matWorld;
			camera_->matView = MathUtility::MakeInverseMatrix(camera_->matWorld);
			camera_->UpdateViewProjection();
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
		// イントロ時以外はカメラの揺れを反映（ゲームオーバー時の撃破衝撃シェイクも適用）
		if (phase_ != Phase::kLeady && !useDebugCamera_)
		{
			if (shake_->IsActive())
			{
				Vector3 offset = shake_->GetOffset();
				// 基準姿勢からオフセットを加算（累積加算による破綻・ブレを防止）
				if (railCamera_)
				{
					camera_->matWorld = railCamera_->GetWorldTransform()->matWorld;
				}
				camera_->matWorld.m[3][0] += offset.x;
				camera_->matWorld.m[3][1] += offset.y;
				camera_->matWorld.m[3][2] += offset.z;

				camera_->matView = MathUtility::MakeInverseMatrix(camera_->matWorld);
				camera_->UpdateViewProjection();
			}
		}
	}

	// ------------------------------------
	// デバッグ用即死ショートカット: [K] キー または ゲームパッドのBackボタンで即座にHPを0にして死亡演出
	// ------------------------------------
	if (phase_ == Phase::kPlay && player_ && player_->IsAlive())
	{
		if (input->TriggerKey(DIK_K) || input->TriggerKey(DIK_DELETE) || input->TriggerButton(XINPUT_GAMEPAD_BACK))
		{
			player_->OnCollision();
		}
	}

	// プレイヤーが死亡している場合はゲームオーバーへ移行
	if (phase_ == Phase::kPlay && player_ && !player_->IsAlive())
	{
		ChangePhase(Phase::kGameOver);
	}

	if (phase_ == Phase::kLeady)
	{
		// ------------------------------------
		// ステージ開始演出フェーズ (kLeady)
		// ゴール手前・高所からスタート地点を見下ろすカメラワーク
		// ------------------------------------
		UpdateIntroCamera();

		// 自機はアニメーションのみ更新
		if (player_)
		{
			player_->UpdateAnimationOnly();
		}

		// ゴールのアニメーション更新
		if (goal_)
		{
			goal_->Update();
		}

		// 背景とレベルオブジェクトの更新
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

		// SPACEキーまたはゲームパッドAボタンで演出スキップ
		if (input->TriggerKey(DIK_SPACE) || input->TriggerButton(XINPUT_GAMEPAD_A))
		{
			ChangePhase(Phase::kPlay);
			return;
		}
	}
	else if (phase_ == Phase::kPlay)
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
		// ステップ1: まず自機の消滅演出（ディゾルブ＋飛散粒子）を見届ける
		// ステップ2: 自機が完全に消滅した後、初めて背景を薄暗くし、GAMEOVER文字とグリッチを表示
		// ------------------------------------

		float dt = TimeManager::GetInstance()->GetDeltaTime();

		// 1. プレイヤーの更新（ディゾルブ消滅を進行させる）
		std::list<EnemyBase*> emptyEnemies;
		player_->Update(emptyEnemies);

		// ================================================================
		// ステップ1: プレイヤーの消滅演出中
		// ================================================================
		if (gameOverStep_ == GameOverStep::kPlayerDisappearing)
		{
			float threshold = player_->GetDissolveThreshold();
			Vector3 playerPos = player_->GetWorldPosition();

			// ----------------------------------------------------------------
			// 崩壊中：機体の視認性を邪魔しない、ごく控えめな火花の散り
			// ----------------------------------------------------------------
			if (threshold < 1.0f)
			{
				// 毎フレーム1個だけ微細なスパークがパチッと散る（機体の崩壊がクリアに見える）
				if (rand() % 2 == 0)
				{
					Vector3 randOffset = {
						(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.8f,
						(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.8f,
						(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.8f
					};
					Vector3 vel = {
						(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 1.5f,
						0.5f + (static_cast<float>(rand()) / RAND_MAX) * 1.0f,
						(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 1.5f
					};
					Vector4 pColor = (threshold < 0.35f)
						? Vector4{ 0.2f, 0.95f, 1.0f, 0.8f }   // シアン火花
						: ((threshold < 0.68f)
							? Vector4{ 1.0f, 0.2f, 0.7f, 0.8f }  // マゼンタ火花
							: Vector4{ 1.0f, 0.9f, 0.3f, 0.8f }); // ゴールド火花

					ParticleManager::GetInstance()->Emit("CircleParticle", playerPos + randOffset, vel, pColor, { 0.08f, 0.08f, 0.08f }, 0.25f, 1);
				}

				// ----------------------------------------------------
				// 第1段階: 外殻（正二十面体フレーム）の崩壊時（上品に8個だけ弾ける）
				// ----------------------------------------------------
				if (threshold >= 0.35f && !isOuterBreakTriggered_)
				{
					isOuterBreakTriggered_ = true;

					// 外殻フレームが割れた瞬間にパチッと散るシアン火花 (8個)
					for (int i = 0; i < 8; ++i)
					{
						float angle = (static_cast<float>(i) / 8.0f) * 6.2831853f;
						float speed = 2.0f + (static_cast<float>(rand()) / RAND_MAX) * 2.0f;
						float pitch = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 1.2f;

						Vector3 vel = {
							std::cos(angle) * std::cos(pitch) * speed,
							std::sin(pitch) * speed,
							std::sin(angle) * std::cos(pitch) * speed
						};
						Vector4 color = ((i % 2) == 0)
							? Vector4{ 0.20f, 0.95f, 1.0f, 0.85f } // シアン
							: Vector4{ 1.0f, 1.0f, 1.0f, 0.90f };   // ホワイト

						ParticleManager::GetInstance()->Emit("CircleParticle", playerPos, vel, color, { 0.10f, 0.10f, 0.10f }, 0.35f, 1);
					}

					// 微小破片キューブ (3個)
					for (int i = 0; i < 3; ++i)
					{
						Vector3 boxVel = {
							(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 3.0f,
							(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 3.0f,
							(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 3.0f
						};
						ParticleManager::GetInstance()->Emit("DigitalGlitchBox", playerPos, boxVel, { 0.9f, 1.0f, 1.0f, 0.8f }, { 0.10f, 0.10f, 0.10f }, 0.35f, 1);
					}

					if (shake_)
					{
						shake_->Start(0.08f, 0.08f);
					}
				}

				// ----------------------------------------------------
				// 第2段階: 中間層（十二面体・ジャイロリング）の崩壊時（10個だけ弾ける）
				// ----------------------------------------------------
				if (threshold >= 0.68f && !isMiddleBreakTriggered_)
				{
					isMiddleBreakTriggered_ = true;

					// マゼンタ火花 (10個)
					for (int i = 0; i < 10; ++i)
					{
						float angle = (static_cast<float>(i) / 10.0f) * 6.2831853f;
						float speed = 2.2f + (static_cast<float>(rand()) / RAND_MAX) * 2.5f;
						float pitch = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 1.4f;

						Vector3 vel = {
							std::cos(angle) * std::cos(pitch) * speed,
							std::sin(pitch) * speed,
							std::sin(angle) * std::cos(pitch) * speed
						};
						Vector4 color = ((i % 2) == 0)
							? Vector4{ 1.0f, 0.15f, 0.75f, 0.85f } // マゼンタ
							: Vector4{ 1.0f, 0.85f, 0.25f, 0.85f }; // ゴールド

						ParticleManager::GetInstance()->Emit("CircleParticle", playerPos, vel, color, { 0.11f, 0.11f, 0.11f }, 0.35f, 1);
					}

					// 微小破片キューブ (4個)
					for (int i = 0; i < 4; ++i)
					{
						Vector3 boxVel = {
							(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 3.5f,
							(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 3.5f,
							(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 3.5f
						};
						ParticleManager::GetInstance()->Emit("DigitalGlitchBox", playerPos, boxVel, { 1.0f, 0.3f, 0.8f, 0.8f }, { 0.11f, 0.11f, 0.11f }, 0.35f, 1);
					}

					if (shake_)
					{
						shake_->Start(0.10f, 0.10f);
					}
				}

				// ----------------------------------------------------
				// 第3段階: 中心核（光核・コア）の消滅時（16個の星屑が散る）
				// ----------------------------------------------------
				if (threshold >= 0.98f && !isCoreBreakTriggered_)
				{
					isCoreBreakTriggered_ = true;

					// コア消滅時の星屑光粒子 (16個)
					for (int i = 0; i < 16; ++i)
					{
						float angle = (static_cast<float>(i) / 16.0f) * 6.2831853f;
						float speed = 2.5f + (static_cast<float>(rand()) / RAND_MAX) * 3.0f;
						float pitch = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 1.5f;

						Vector3 vel = {
							std::cos(angle) * std::cos(pitch) * speed,
							std::sin(pitch) * speed,
							std::sin(angle) * std::cos(pitch) * speed
						};
						Vector4 color;
						int cType = i % 3;
						if (cType == 0) color = { 1.0f, 0.95f, 0.30f, 0.90f };      // ゴールド
						else if (cType == 1) color = { 0.20f, 1.0f, 1.0f, 0.90f };   // シアン
						else color = { 1.0f, 1.0f, 1.0f, 0.90f };                   // ホワイト

						ParticleManager::GetInstance()->Emit("CircleParticle", playerPos, vel, color, { 0.12f, 0.12f, 0.12f }, 0.45f, 1);
					}

					// 微小破片キューブ (5個)
					for (int i = 0; i < 5; ++i)
					{
						Vector3 boxVel = {
							(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 4.0f,
							(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 4.0f,
							(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 4.0f
						};
						ParticleManager::GetInstance()->Emit("DigitalGlitchBox", playerPos, boxVel, { 1.0f, 0.95f, 0.4f, 0.85f }, { 0.12f, 0.12f, 0.12f }, 0.40f, 1);
					}

					if (shake_)
					{
						shake_->Start(0.10f, 0.10f);
					}
				}
			}
			else
			{
				// 自機が完全にディゾルブしきった後の余韻待機タイマー
				playerDisappearTimer_ += dt;

				if (playerDisappearTimer_ >= kPlayerDisappearPostWait)
				{
					// 自機の消滅演出が完全に終了！
					// ここで初めてゲームオーバーUI（背景暗転＋GAMEOVER文字＋グリッチ）を開始
					gameOverStep_ = GameOverStep::kGameOverUI;
					if (gameOverUI_)
					{
						gameOverUI_->Start();
					}
				}
			}
		}
		// ================================================================
		// ステップ2: 自機消滅完了後 ➔ GAMEOVER演出 & 操作受付
		// ================================================================
		else if (gameOverStep_ == GameOverStep::kGameOverUI)
		{
			if (gameOverUI_)
			{
				gameOverUI_->Update();

				// リトライ（ゲームプレイ再開）要求: CRT画面遷移演出を挟む
				if (gameOverUI_->IsRetryRequested())
				{
					if (!CrtTransition::GetInstance()->IsTransitioning())
					{
						input->SetShake(0.0f, 0.0f);
						CrtTransition::GetInstance()->StartTransition("GAMEPLAY", PostProcessRenderer::PostProcessMode::kNormal);
					}
					return;
				}
				// タイトルへ戻る要求: CRT画面遷移演出を挟む
				if (gameOverUI_->IsTitleRequested())
				{
					if (!CrtTransition::GetInstance()->IsTransitioning())
					{
						input->SetShake(0.0f, 0.0f);
						CrtTransition::GetInstance()->StartTransition("TITLE", PostProcessRenderer::PostProcessMode::kGlitch);
					}
					return;
				}
			}
		}

		// ------------------------------------
		// パーティクルの更新（自機崩壊パーティクルの進行・GPU転送）
		// ------------------------------------
		ParticleManager::GetInstance()->Update(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());
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

	// 自死案内スプライトの更新
	if (uiKillHint_)
	{
		uiKillHint_->Update();
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

	if (phase_ == Phase::kPlay)
	{
		lockOn_->Draw();
	}

	// ------------------------------------
	// パーティクル描画
	// ------------------------------------
	ParticleManager::GetInstance()->Draw();

	// ------------------------------------
	// UI (イントロ演出中とゲームオーバー中は通常UIを非表示)
	// ------------------------------------

	if (phase_ != Phase::kLeady && phase_ != Phase::kGameOver)
	{
		uiPlayerHp_->Draw();

		// スコアUIの描画
		for (auto& digitSprite : uiScoreDigits_)
		{
			digitSprite->Draw();
		}

		// 画面中央の自死案内スプライトUI（[K] キーで自滅）を描画
		if (uiKillHint_)
		{
			uiKillHint_->Draw();
		}
	}

	// ------------------------------------
	// ゲームオーバー演出（自機消滅完了後のみ暗幕・GAMEOVER文字・グリッチUIを描画）
	// ------------------------------------
	if (phase_ == Phase::kGameOver && gameOverStep_ == GameOverStep::kGameOverUI && gameOverUI_)
	{
		gameOverUI_->Draw();
	}

	// ------------------------------------
	// フェードアウト (最前面に描画: クリア時など)
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
	uiKillHint_.reset();

	if (gameOverUI_)
	{
		gameOverUI_->Reset();
		gameOverUI_.reset();
	}

	// 画面遷移中（CrtTransition実行中）以外のみ通常描画へ戻す
	if (!CrtTransition::GetInstance()->IsTransitioning())
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
				shake_->Start(0.18f, 0.16f);
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
		introCameraTimer_ = 0.0f;
		playerMaterializeTimer_ = 0.0f;
		if (railCamera_)
		{
			railCamera_->SetIsPlaying(false);
			railCamera_->SetSplineTime(0.0f);
		}
		if (player_)
		{
			player_->SetMaterializeProgress(0.0f);
		}
		playerMaterializedEffectTriggered_ = false;
		UpdateIntroCamera();
		break;
	case Phase::kPlay:
		if (camera_)
		{
			camera_->SetFovY(introEndFov_);
		}
		if (player_)
		{
			player_->SetMaterializeProgress(1.0f);
		}
		// レールカメラの再生を開始
		if (railCamera_)
		{
			railCamera_->SetIsPlaying(true);
			railCamera_->SetSplineTime(0.0f);
			if (!useDebugCamera_)
			{
				railCamera_->Update(true);
			}
		}
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
		// 2. 自機消滅ステップの初期化（まずは自機が消滅する演出を見届ける）
		gameOverStep_ = GameOverStep::kPlayerDisappearing;
		playerDisappearTimer_ = 0.0f;
		isOuterBreakTriggered_ = false;
		isMiddleBreakTriggered_ = false;
		isCoreBreakTriggered_ = false;
		isPlayerDestroyEffectTriggered_ = false;
		if (gameOverUI_)
		{
			gameOverUI_->Reset();
		}

		// 3. 被弾・機能停止の初期スパーク演出（自機の被弾・火花・ディゾルブ崩壊開始）
		if (!isPlayerDestroyEffectTriggered_ && player_)
		{
			isPlayerDestroyEffectTriggered_ = true;

			// 自機のディゾルブ（消滅演出）を確実に開始
			player_->SetHP(0.0f);
			player_->SetDissolveEnable(true);

			Vector3 playerPos = player_->GetWorldPosition();

			// 被弾スパーク粒子 (24個)
			for (int i = 0; i < 24; ++i)
			{
				float angle = (static_cast<float>(i) / 24.0f) * 6.2831853f;
				float speed = 3.5f + (static_cast<float>(rand()) / RAND_MAX) * 4.5f;
				float pitchAngle = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 1.6f;

				Vector3 burstVel = {
					std::cos(angle) * std::cos(pitchAngle) * speed,
					std::sin(pitchAngle) * speed,
					std::sin(angle) * std::cos(pitchAngle) * speed
				};

				Vector4 color = ((i % 2) == 0)
					? Vector4{ 1.0f, 0.25f, 0.30f, 1.0f }  // クリムゾンレッド
					: Vector4{ 0.20f, 0.90f, 1.0f, 1.0f }; // シアン

				ParticleManager::GetInstance()->Emit("CircleParticle", playerPos, burstVel, color, { 0.20f, 0.20f, 0.20f }, 0.45f, 1);
			}

			// デジタルグリッチボックスのスパーク (10個)
			for (int i = 0; i < 10; ++i)
			{
				Vector3 boxVel = {
					(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 5.0f,
					(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 5.0f,
					(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 5.0f
				};
				ParticleManager::GetInstance()->Emit("DigitalGlitchBox", playerPos, boxVel, { 1.0f, 0.30f, 0.35f, 1.0f }, { 0.18f, 0.18f, 0.18f }, 0.35f, 1);
			}

			// 初動の被弾カメラマイクロシェイク
			if (shake_)
			{
				shake_->Start(0.15f, 0.12f);
			}
		}
		// 4. 通常フェードアウトの初期化
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

void GamePlayScene::UpdateIntroCamera() {
	if (!railCamera_ || !camera_) return;

	float dt = TimeManager::GetInstance()->GetDeltaTime();
	introCameraTimer_ += dt;

	// コースの始点・終点（ゴール）を取得
	const auto& controlPoints = railCamera_->GetControlPoints();
	Vector3 startPos = controlPoints.empty() ? Vector3{ 0.0f, 0.0f, 0.0f } : controlPoints.front();
	Vector3 goalPos = controlPoints.empty() ? Vector3{ 0.0f, 0.0f, 150.0f } : controlPoints.back();
	Vector3 stageCenter = (startPos + goalPos) * 0.5f;

	// 通常プレイ時の初期カメラ座標・回転（スタート位置）
	Vector3 playCamPos = startPos;
	Vector3 playCamRot = { 0.0f, 0.0f, 0.0f };

	// ----------------------------------------------------
	// 1. カメラワーク制御
	// ゴール上空から旋回し、自機背後・通常プレイ視界へスムーズに移動
	// ----------------------------------------------------
	if (introCameraTimer_ < introCameraDuration_) {
		float rawT = (std::min)(1.0f, introCameraTimer_ / (std::max)(0.01f, introCameraDuration_));

		// 緩やかな加減速（Cubic Ease In-Out）
		float t = (rawT < 0.5f)
			? 4.0f * rawT * rawT * rawT
			: 1.0f - std::pow(-2.0f * rawT + 2.0f, 3.0f) / 2.0f;

		// FOV（視野角）の補間: 開始時は広角でステージ全体を捉え、接近とともに通常へ
		float currentFov = introStartFov_ * (1.0f - t) + introEndFov_ * t;
		camera_->SetFovY(currentFov);

		// カメラ座標（3次ベジェ曲線による外側旋回オービット軌道）
		Vector3 p0 = {
			goalPos.x + introStartOffset_.x,
			goalPos.y + introStartOffset_.y,
			goalPos.z + introStartOffset_.z
		};
		Vector3 p1 = {
			stageCenter.x + introSideOffset_.x,
			stageCenter.y + introSideOffset_.y,
			stageCenter.z + introSideOffset_.z
		};
		Vector3 p2 = {
			startPos.x + introApproachOffset_.x,
			startPos.y + introApproachOffset_.y,
			startPos.z + introApproachOffset_.z
		};
		Vector3 p3 = playCamPos;

		// 3次ベジェ補間
		float omt = 1.0f - t;
		Vector3 camPos = p0 * (omt * omt * omt)
			+ p1 * (3.0f * omt * omt * t)
			+ p2 * (3.0f * omt * t * t)
			+ p3 * (t * t * t);

		// 注視点（Look Target）の補間
		Vector3 targetStage = { stageCenter.x, stageCenter.y + 4.0f, stageCenter.z + 10.0f };
		Vector3 targetPlay = { startPos.x, startPos.y, startPos.z + 80.0f };

		// 前半（rawT < 0.35）はステージ中央を注視、0.35〜0.9 でプレイ注視点へスムーズに移行
		float targetT = (std::clamp)((rawT - 0.35f) / 0.55f, 0.0f, 1.0f);
		float targetEase = targetT * targetT * (3.0f - 2.0f * targetT); // Smoothstep
		Vector3 lookTarget = targetStage * (1.0f - targetEase) + targetPlay * targetEase;

		Vector3 toTarget = lookTarget - camPos;
		float distXZ = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
		float lookPitch = std::atan2(-toTarget.y, (std::max)(0.001f, distXZ));
		float lookYaw = std::atan2(toTarget.x, toTarget.z);

		// 終盤（rawT >= 0.8f）で最終的なプレイ姿勢（正面）へ完全同期
		Vector3 finalRot;
		if (rawT < 0.8f) {
			finalRot = { lookPitch, lookYaw, 0.0f };
		} else {
			float blendRaw = (rawT - 0.8f) / 0.2f;
			float blendT = blendRaw * blendRaw * (3.0f - 2.0f * blendRaw);

			float currentPitch = lookPitch * (1.0f - blendT) + playCamRot.x * blendT;

			float diffYaw = playCamRot.y - lookYaw;
			while (diffYaw < -3.14159265f) diffYaw += 6.2831853f;
			while (diffYaw > 3.14159265f) diffYaw -= 6.2831853f;
			float currentYaw = lookYaw + diffYaw * blendT;

			finalRot = { currentPitch, currentYaw, 0.0f };
		}

		if (!useDebugCamera_) {
			camera_->SetTranslate(camPos);
			camera_->SetRotate(finalRot);
			camera_->CalculateMatrix();
		}
	} else {
		// カメラ移動完了後はスタート地点（通常プレイ視界）に完全固定
		camera_->SetFovY(introEndFov_);
		if (!useDebugCamera_) {
			camera_->SetTranslate(playCamPos);
			camera_->SetRotate(playCamRot);
			camera_->CalculateMatrix();
		}
	}

	// ----------------------------------------------------
	// 2. プレイヤーのマテリアライズ（出現演出）
	// カメラがスタート地点に移動しきる少し前（introCameraDuration_ - playerMaterializeLeadTime_）から生成開始！
	// ----------------------------------------------------
	float spawnStartTime = (std::max)(0.0f, introCameraDuration_ - playerMaterializeLeadTime_);

	if (introCameraTimer_ < spawnStartTime) {
		// まだ生成開始タイミングに達していない場合は完全非表示
		if (player_) {
			player_->SetMaterializeProgress(0.0f);
		}
	} else {
		// カメラが到着しきる少し前から形成進行
		playerMaterializeTimer_ += dt;
		float matProgress = (std::clamp)(playerMaterializeTimer_ / (std::max)(0.01f, playerMaterializeDuration_), 0.0f, 1.0f);

		if (player_) {
			player_->SetMaterializeProgress(matProgress);
			Vector3 playerPos = player_->GetWorldPosition();

			// A. 形成進行中（0.01 < matProgress < 0.98）：
			// 周囲の球面上から中心へ向かって、大量のサイバー光粒子＆グリッチボックスが高速収束！
			if (matProgress > 0.01f && matProgress < 0.98f) {
				// 1フレームあたり 6 個の光粒子ストリーム（渦を巻いて中心へ収束）
				for (int i = 0; i < 6; ++i) {
					float theta = (static_cast<float>(rand()) / RAND_MAX) * 6.2831853f;
					float phi = (static_cast<float>(rand()) / RAND_MAX) * 3.1415926f;
					float radius = 1.8f + (static_cast<float>(rand()) / RAND_MAX) * 2.2f;

					Vector3 randOffset = {
						radius * std::sin(phi) * std::cos(theta),
						radius * std::cos(phi),
						radius * std::sin(phi) * std::sin(theta)
					};
					Vector3 pPos = playerPos + randOffset;
					Vector3 tangent = { -randOffset.z * 1.5f, 0.0f, randOffset.x * 1.5f };
					Vector3 pVel = randOffset * -3.2f + tangent;

					int colorType = rand() % 3;
					Vector4 color;
					if (colorType == 0) color = { 0.0f, 1.0f, 1.0f, 0.9f };       // ネオンシアン
					else if (colorType == 1) color = { 1.0f, 0.05f, 0.85f, 0.9f }; // ネオンマゼンタ
					else color = { 1.0f, 0.92f, 0.25f, 0.95f };                   // 超高輝度ゴールド

					ParticleManager::GetInstance()->Emit("CircleParticle", pPos, pVel, color, { 0.16f, 0.16f, 0.16f }, 0.45f, 1);
				}

				// デジタルグリッチボックスの明滅
				if (rand() % 2 == 0) {
					Vector3 randOffset = {
						(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.2f,
						(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.2f,
						(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.2f
					};
					ParticleManager::GetInstance()->Emit("DigitalGlitchBox", playerPos + randOffset, { 0.0f, 0.6f, 0.0f }, { 0.2f, 1.0f, 1.0f, 0.95f }, { 0.18f, 0.18f, 0.18f }, 0.35f, 1);
				}
			}

			// B. 形成完了（matProgress >= 0.98f）の瞬間！
			// 全方位デジタル粒子バースト ＋ カメラシェイク！
			if (matProgress >= 0.98f && !playerMaterializedEffectTriggered_) {
				playerMaterializedEffectTriggered_ = true;

				// 1. 全方位へ光粒子とグリッチボックスが弾けるバースト (36個)
				for (int i = 0; i < 36; ++i) {
					float angle = (static_cast<float>(i) / 36.0f) * 6.2831853f;
					float speed = 4.0f + (static_cast<float>(rand()) / RAND_MAX) * 6.0f;
					float pitchAngle = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 1.5f;

					Vector3 burstVel = {
						std::cos(angle) * std::cos(pitchAngle) * speed,
						std::sin(pitchAngle) * speed,
						std::sin(angle) * std::cos(pitchAngle) * speed
					};

					Vector4 color = ((i % 2) == 0)
						? Vector4{ 0.0f, 1.0f, 1.0f, 1.0f }   // シアン
						: Vector4{ 1.0f, 0.2f, 0.9f, 1.0f }; // マゼンタ

					ParticleManager::GetInstance()->Emit("CircleParticle", playerPos, burstVel, color, { 0.22f, 0.22f, 0.22f }, 0.55f, 1);
				}

				// デジタルグリッチボックスの拡散 (12個)
				for (int i = 0; i < 12; ++i) {
					Vector3 boxVel = {
						(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 8.0f,
						(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 8.0f,
						(static_cast<float>(rand()) / RAND_MAX - 0.5f) * 8.0f
					};
					ParticleManager::GetInstance()->Emit("DigitalGlitchBox", playerPos, boxVel, { 1.0f, 0.95f, 0.3f, 1.0f }, { 0.22f, 0.22f, 0.22f }, 0.45f, 1);
				}

				// 2. 手応えのあるカメラマイクロシェイク
				if (shake_) {
					shake_->Start(0.22f, 0.40f);
				}
			}
		}

		// カメラ移動が完了し、かつ自機形成も完了（＋余韻待機時間）したらプレイフェーズへ移行
		if (introCameraTimer_ >= introCameraDuration_ &&
			playerMaterializeTimer_ >= playerMaterializeDuration_ + kPlayerMaterializePostWait) {
			ChangePhase(Phase::kPlay);
		}
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
		ImGui::SameLine();
		if (ImGui::Button("Debug: Kill Player [K]")) {
			if (player_) {
				player_->OnCollision();
			}
		}
	}

	if (ImGui::CollapsingHeader("Stage Intro Camera", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::SliderFloat("Camera Move Duration (s)", &introCameraDuration_, 1.0f, 10.0f, "%.1f");
		ImGui::SliderFloat("Player Spawn Lead Time (s)", &playerMaterializeLeadTime_, 0.0f, 3.0f, "%.2f");
		ImGui::SliderFloat("Player Materialize Duration (s)", &playerMaterializeDuration_, 0.5f, 5.0f, "%.1f");
		ImGui::SliderFloat("Start FOV", &introStartFov_, 0.3f, 1.2f, "%.2f rad");
		ImGui::DragFloat3("p0: Start (Goal Offset)", &introStartOffset_.x, 0.5f, -200.0f, 200.0f);
		ImGui::DragFloat3("p1: Side (Mid Offset)", &introSideOffset_.x, 0.5f, -200.0f, 200.0f);
		ImGui::DragFloat3("p2: Approach (Start Offset)", &introApproachOffset_.x, 0.5f, -200.0f, 200.0f);

		if (ImGui::Button("Replay Intro Camera"))
		{
			ChangePhase(Phase::kLeady);
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

	if (ImGui::CollapsingHeader("Player Reticle (Cyberpunk HUD)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (player_ && player_->GetReticle())
		{
			Reticle* reticle = player_->GetReticle();
			int themeIdx = static_cast<int>(reticle->GetColorTheme());
			const char* themeItems[] = {
				"1. Cyber Amber & Solar White (最高視認性・対比色オレンジ/琥珀&白)",
				"2. Electric Lime & Citron (超蛍光ライムグリーン&イエロー)",
				"3. Neon Gold & Cyber Magenta (ゴールド&マゼンタ華麗ツートン)",
				"4. Solar Crimson & White (プラズマクリムゾン戦闘機HUD)"
			};
			if (ImGui::Combo("HUD Color Theme", &themeIdx, themeItems, IM_ARRAYSIZE(themeItems)))
			{
				reticle->SetColorTheme(static_cast<Reticle::ColorTheme>(themeIdx));
			}
		}
	}

	// CRT画面遷移デバッグ
	CrtTransition::GetInstance()->UpdateImGui();

	ImGui::End();
#endif
}