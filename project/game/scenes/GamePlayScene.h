#pragma once

#include "IScene.h"

#include <Vector3.h>

#include <memory>
#include <vector>
#include <string>

class Object3d;
class Camera;
class DebugCamera;
class Player;
class RusherEnemy;

class GamePlayScene : public IScene {
public:
	GamePlayScene();
	~GamePlayScene();

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

private:
	// -----------------------
	// カメラ
	// -----------------------
	std::unique_ptr<Camera> camera_;

	// 初期位置
	static inline const Vector3 kInitialCameraPos = {0.0f, 0.0f, -20.0f};

	// デバッグカメラ
	std::unique_ptr<DebugCamera> debugCamera_;
	bool useDebugCamera_ = true;

	// -----------------------
	// 自機
	// -----------------------
	std::unique_ptr<Player> player_;

	// モデル
	std::unique_ptr<Object3d> playerModel_;
	// 自機の弾のモデル
	std::unique_ptr<Object3d> playerBulletModel_;

	// 初期位置
	static inline const Vector3 kInitialPlayerPos = {0.0f, 0.0f, 0.0f};

	// -----------------------
	// 敵
	// -----------------------

	// 同時に存在させる敵の最大数
	static inline const int kMaxEnemyCount = 3;

	// 敵のモデル名（統一）
	static inline const std::string kEnemyModelName = "sphere";

	// スポーン範囲
	static inline const float kSpawnRangeX = 10.0f;
	static inline const float kSpawnRangeY = 0.0f;  // Y は地面に合わせて固定
	static inline const float kSpawnZ = 15.0f; // Z は固定（プレイヤーの前方）

	std::vector<std::unique_ptr<RusherEnemy>> enemies_;

	/// <summary>
	/// 敵を1体スポーンする（ランダム位置）
	/// </summary>
	void SpawnEnemy();

private:
	/// <summary>
	/// ImGuiの描画
	/// </summary>
	void UpdateImGui();
};