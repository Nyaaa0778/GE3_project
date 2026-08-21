#pragma once

#include "IScene.h"
#include "WorldTransform.h"
#include "LevelData.h"

#include <memory>
#include <vector>
#include <list>

class Object3d;
class Camera;
class DebugCamera;
class RailCameraController;
class Player;
class Level;
class Skydome;
class EnemyBase;
class Collider;
class Shockwave;
class Shake;
class LockOn;
class Goal;
class Sprite;
enum class DroneFlightPattern;

class GamePlayScene : public IScene {
public:
	enum class Phase {
		kLeady,    // スタート準備
		kPlay,     // ゲームプレイ
		kClear,    // クリア
		kGameOver, // ゲームオーバー
	};

	Phase phase_ = Phase::kLeady;
public:
	GamePlayScene();
	~GamePlayScene();

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

private:
	// ------------------------------------
	// レベル
	// ------------------------------------

	std::unique_ptr<Level> level_;

	// ------------------------------------
	// 自機
	// ------------------------------------

	std::unique_ptr<Player> player_;
	// モデル
	std::unique_ptr<Object3d> playerModel_;

	// ------------------------------------
	// ロックオン
	// ------------------------------------

	std::unique_ptr<LockOn> lockOn_;

	// ------------------------------------
	// カメラ
	// ------------------------------------

	std::unique_ptr<Camera> camera_;

	// レールカメラ
	std::unique_ptr<RailCameraController> railCamera_;

	// デバッグカメラ
	std::unique_ptr<DebugCamera> debugCamera_;
	bool useDebugCamera_ = false;

	// ------------------------------------
	// 天球
	// ------------------------------------

	std::unique_ptr<Skydome> skydome_;

	// モデル
	std::unique_ptr<Object3d> skydomeModel_;

	// ------------------------------------
	// 敵
	// ------------------------------------

	std::unique_ptr<Object3d> enemyModel_;
	std::list<std::unique_ptr<EnemyBase>> enemies_;
	std::vector<LevelData::SpawnerData> pendingEnemies_;

	// ------------------------------------
	// エフェクト
	// ------------------------------------

	std::list<std::unique_ptr<Shockwave>> shockwaves_;
	std::unique_ptr<Shake> shake_;

	// ------------------------------------
	// ゴール
	// ------------------------------------
	std::unique_ptr<Goal> goal_;
	bool isGoalReached_ = false;

	// ------------------------------------
	// UI
	// ------------------------------------

	// プレイヤーのHP
	std::unique_ptr<Sprite> uiPlayerHp_;

	// スコア
	int score_ = 0;
	int prevScore_ = -1;

	// スコアUI
	static constexpr int kMaxScoreDigits = 5;
	std::vector<std::unique_ptr<Sprite>> uiScoreDigits_;


private:
	// ドローン編隊の生成
	void SpawnDroneFormation(const Vector3& basePos, DroneFlightPattern pattern, int count = 4);

	// 全ての衝突判定をチェック
	void CheckAllCollisions();

	// フェーズチェンジ
	void ChangePhase(Phase nextPhase);

	// ImGuiの更新
	void UpdateImGui();
};
