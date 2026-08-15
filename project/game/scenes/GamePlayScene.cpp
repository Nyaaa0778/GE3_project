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
#include "Shockwave.h"
#include "Collider.h"
#include "Shake.h"
#include "Goal.h"
#include "TimeManager.h"
#include "Sprite.h"

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

	// Spawnerデータから "Enemy" という名前が含まれるものをすべて取得して出現待ちリストに格納
	pendingEnemies_ = level_->GetSpawners("Enemy");

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

	// パーティクルグループの作成と初期クリア
	ParticleManager::GetInstance()->CreateParticleGroup("CircleParticle", "resources/sprites/circle.png", ParticleManager::ParticleShape::kPlane);
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

	if (railCamera_)
	{
		railCamera_->Update(!useDebugCamera_);
	}
	if (useDebugCamera_)
	{
		debugCamera_->Update(camera_.get());
	}

#ifdef USE_IMGUI
	if (railCamera_)
	{
		railCamera_->DrawDebugSpline();
	}
#endif

	// 画面シェイクの更新と適用
	if (shake_)
	{
		shake_->Update(TimeManager::GetInstance()->GetDeltaTime());
		if (shake_->IsActive() && !useDebugCamera_)
		{
			Vector3 offset = shake_->GetOffset();
			camera_->matWorld.m[3][0] += offset.x;
			camera_->matWorld.m[3][1] += offset.y;
			camera_->matWorld.m[3][2] += offset.z;

			camera_->matView = MathUtility::MakeInverseMatrix(camera_->matWorld);
			camera_->UpdateViewProjection();
		}
	}

	if (player_->IsAlive())
	{

		// ------------------------------------
		// ゴールの更新・アニメーション
		// ------------------------------------
		if (goal_)
		{
			goal_->Update();
		}

		// ------------------------------------
		// ゴール到達判定とシーン遷移
		// ------------------------------------
		if (isGoalReached_)
		{
			Input* input = Input::GetInstance();
			if (input->TriggerKey(DIK_RETURN) || input->TriggerButton(XINPUT_GAMEPAD_A))
			{
				SceneManager::GetInstance()->ChangeScene("TITLE");
				return;
			}
		} else
		{
			// 自機との衝突判定によるゴール到達チェック
			if (player_ && goal_)
			{
				if (Collision::CheckCollision(player_.get(), goal_.get()))
				{
					isGoalReached_ = true;
					if (railCamera_)
					{
						railCamera_->SetIsPlaying(false);
					}
				}
			}

			// カメラがレール末尾に到達したことによるゴール到達チェック
			if (railCamera_ && !railCamera_->GetIsLoop())
			{
				float maxTime = static_cast<float>((std::max) (0ULL, railCamera_->GetControlPoints().size()) - 1);
				if (railCamera_->GetSplineTime() >= maxTime)
				{
					isGoalReached_ = true;
					railCamera_->SetIsPlaying(false);
				}
			}
		}

		// ------------------------------------
		// 自機 & 敵 & 衝突判定 (ゴール未到達時のみ更新)
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
						auto enemy = std::make_unique<RusherEnemy>();
						enemy->Initialize(enemyModel_.get(), camera_.get(), it->translation, player_.get());
						enemy->GetWorldTransform().rotation = it->rotation;
						enemy->GetWorldTransform().scale = it->scaling;
						enemies_.push_back(std::move(enemy));

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
				activeEnemies.push_back(enemy.get());
			}
			player_->Update(activeEnemies);

			for (auto& enemy : enemies_)
			{
				enemy->Update();
			}

			CheckAllCollisions();

			for (auto it = enemies_.begin(); it != enemies_.end(); )
			{
				if (!(*it)->IsAlive())
				{
					// スコア加算
					score_ += (*it)->GetScore();

					if (dynamic_cast<RusherEnemy*>(it->get()))
					{
						auto shockwave = std::make_unique<Shockwave>();
						shockwave->Initialize(camera_.get(), (*it)->GetWorldPosition());
						shockwaves_.push_back(std::move(shockwave));
					}
					it = enemies_.erase(it);
				} else
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
		// 天球
		// ------------------------------------

		skydome_->Update();

		// ------------------------------------
		// パーティクルの更新
		// ------------------------------------
		ParticleManager::GetInstance()->Update(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());

		// HPが20以下の時に Vignetting 赤点滅を適用
		if (player_->GetHP() <= 20.0f)
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
			// HPが20より大きくなったら Vignetting モードを解除して通常状態にする
			if (PostProcessRenderer::GetInstance()->GetMode() == PostProcessRenderer::PostProcessMode::kVignetting)
			{
				PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kNormal);
			}
		}
	}
	else
	{
		// ------------------------------------
		// プレイヤー死亡時（ゲーム全体の動きを止め、プレイヤーをディゾルブ消滅させる）
		// ------------------------------------
		// レールカメラの動きを止める
		if (railCamera_)
		{
			railCamera_->SetIsPlaying(false);
		}

		// プレイヤーの更新（ディゾルブを進行させる）
		std::list<EnemyBase*> activeEnemies;
		player_->Update(activeEnemies);

		// ディゾルブ消滅が完了したら、ENTERキー/Aボタンでタイトルへ戻れるようにする
		if (player_->GetDissolveThreshold() >= 1.0f)
		{
			Input* input = Input::GetInstance();
			if (input->TriggerKey(DIK_RETURN) || input->TriggerButton(XINPUT_GAMEPAD_A))
			{
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
}


void GamePlayScene::Finalize() {}

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
		//phase_
		break;
	case Phase::kPlay:
		break;
	case Phase::kClear:
		break;
	case Phase::kGameOver:
		break;
	}
}

void GamePlayScene::UpdateImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("デバッグウィンドウ");

	// FPSを表示
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Text("Score: %d", score_);

	ImGui::End();
#endif
}