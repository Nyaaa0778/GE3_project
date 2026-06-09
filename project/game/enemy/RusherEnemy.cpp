#include "RusherEnemy.h"
#include "../player/Player.h"
#include <TimeManager.h>
#include <MathUtility.h>
#include <cmath>

using namespace MathUtility;

RusherEnemy::RusherEnemy(const EnemyStatus& initialStatus) : EnemyBase(initialStatus) {}

void RusherEnemy::Initialize(Camera* camera, const Vector3& pos, const std::string& modelName, const WorldTransform* parentTransform) {
	EnemyBase::Initialize(camera, pos, modelName, parentTransform);
	state_ = State::kApproach;
	stateTimer_ = 0.0f;
	rushVelocity_ = {0.0f, 0.0f, 0.0f};
}

void RusherEnemy::Update(Player* player) {
	if (!isAlive_) return;

	float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
	stateTimer_ += deltaTime;

	Vector3 playerPos = player ? player->GetWorldPos() : Vector3{0.0f, 0.0f, 0.0f};

	switch (state_) {
	case State::kApproach: {
		// プレイヤーの方向へゆっくり近づく（追尾）
		Vector3 toPlayer = playerPos - pos_;
		float distance = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y + toPlayer.z * toPlayer.z);
		if (distance > 0.0f) {
			Vector3 direction = { toPlayer.x / distance, toPlayer.y / distance, toPlayer.z / distance };
			pos_.x += direction.x * status_.speed * kApproachSpeedMultiplier;
			pos_.y += direction.y * status_.speed * kApproachSpeedMultiplier;
			pos_.z += direction.z * status_.speed * kApproachSpeedMultiplier;
		}

		// 一定時間経過するか、プレイヤーとの距離が一定以下になったら突撃フェーズへ移行
		if (stateTimer_ >= kApproachDuration || distance <= kRushTriggerDistance) {
			state_ = State::kRush;
			stateTimer_ = 0.0f;

			// 突撃する瞬間のプレイヤーへの方向をロックオン
			Vector3 rushDir = playerPos - pos_;
			float rushDist = std::sqrt(rushDir.x * rushDir.x + rushDir.y * rushDir.y + rushDir.z * rushDir.z);
			if (rushDist > 0.0f) {
				rushVelocity_ = {
					(rushDir.x / rushDist) * status_.speed * kRushSpeedMultiplier,
					(rushDir.y / rushDist) * status_.speed * kRushSpeedMultiplier,
					(rushDir.z / rushDist) * status_.speed * kRushSpeedMultiplier
				};
			} else {
				// プレイヤーと全く同じ位置にいる場合は、適当に前方に突撃
				rushVelocity_ = { 0.0f, 0.0f, status_.speed * kRushSpeedMultiplier };
			}
		}
		break;
	}
	case State::kRush: {
		// ロックオンした方向へ直進
		pos_.x += rushVelocity_.x;
		pos_.y += rushVelocity_.y;
		pos_.z += rushVelocity_.z;

		// 突撃開始から3秒経過したら死亡（消滅）とする
		if (stateTimer_ >= 3.0f) {
			Die();
		}
		break;
	}
	}

	// 基底クラスのUpdate（model_の更新等）を呼ぶ
	EnemyBase::Update(player);
}
