#pragma once
#include "EnemyBase.h"

class Player;

class RusherEnemy : public EnemyBase {
private:
	enum class Phase {
		kIdle,     // 待機状態
		kApproach, // 接近状態
		kRush,     // 突進状態
	};

	Phase phase_ = Phase::kIdle;

public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(Object3d* model, Camera* camera, const Vector3& pos, const Player* player);

	/// <summary>
	/// 更新
	/// </summary>
	void Update() override;

	/// <summary>
	/// 衝突時の処理
	/// </summary>
	void OnCollision() override;

	/// <summary>
	/// スコア値取得
	/// </summary>
	int GetScore() const override { return 250; }

private:
	// 各フェーズの更新処理
	void UpdateIdle();
	void UpdateApproach();
	void UpdateRush();

private:
	// 追跡対象のプレイヤー
	const Player* player_ = nullptr;

	// 移動速度
	static constexpr float kSpeed = 0.01f;
	// 突進速度
	static constexpr float kRushSpeed = 0.5f;

	// 状態遷移のしきい値距離
	static constexpr float kApproachRange = 40.0f;
	static constexpr float kRushRange = 15.0f;

	// 突進方向
	Vector3 rushDirection_ = {};

	// 当たり判定の大きさ
	static constexpr Vector3 kCollisionSize = {1.0f, 1.0f, 1.0f};

	// 突進タイマー
	float rushTimer_ = 0.0f;
	static constexpr float kMaxRushTime = 1.5f; // 1.5秒間突進
};

