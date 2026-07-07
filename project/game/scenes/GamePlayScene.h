#pragma once

#include "IScene.h"
#include "WorldTransform.h"
#include "../goal/Goal.h"

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

class GamePlayScene : public IScene {
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

private:
	// 全ての衝突判定をチェック
	void CheckAllCollisions();

	void UpdateImGui();
};
