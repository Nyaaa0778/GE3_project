#pragma once
#include "EnemyBase.h"

class RusherEnemy : public EnemyBase {
public:
	enum class State {
		kApproach, // プレイヤーに接近（追尾）
		kRush      // 突撃（直線移動）
	};

private:
	State state_ = State::kApproach;
	float stateTimer_ = 0.0f;

	// 突撃時の方向ベクトル（速度）
	Vector3 rushVelocity_ = {0.0f, 0.0f, 0.0f};

	// パラメータ
	static inline const float kApproachDuration = 2.0f; // 追尾時間（秒）
	static inline const float kRushSpeedMultiplier = 3.0f; // 突撃時の速度倍率
	static inline const float kApproachSpeedMultiplier = 0.5f; // 接近時の速度倍率
	static inline const float kRushTriggerDistance = 35.0f; // 突撃を開始する距離

public:
	RusherEnemy(const EnemyStatus& initialStatus);
	~RusherEnemy() override = default;

	void Initialize(Camera* camera, const Vector3& pos, const std::string& modelName, const WorldTransform* parentTransform = nullptr) override;
	void Update(Player* player) override;
};
