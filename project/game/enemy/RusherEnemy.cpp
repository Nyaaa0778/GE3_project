#include "RusherEnemy.h"
#include "Player.h"
#include <MathUtility.h>
#include <MyEngine.h>

using namespace MathUtility;

void RusherEnemy::Initialize(Object3d* model, Camera* camera, const Vector3& pos, const Player* player) {
	// 基底クラスの初期化
	EnemyBase::Initialize(model, camera, pos);
	player_ = player;
	phase_ = Phase::kIdle;
	rushTimer_ = 0.0f;

	// コライダーの初期設定 (AABB, サイズ 1.5)
	SetShape(ColliderShape::kAABB);
	SetAABB({ kCollisionSize });
}

void RusherEnemy::Update() {
	if (!IsAlive()) {
		EnemyBase::Update();
		return;
	}

	if (player_) {
		switch (phase_) {
		case Phase::kIdle:
			UpdateIdle();
			break;
		case Phase::kApproach:
			UpdateApproach();
			break;
		case Phase::kRush:
			UpdateRush();
			break;
		}
	}

	// 基底クラスのUpdate
	EnemyBase::Update();
}

void RusherEnemy::UpdateIdle() {
	Vector3 playerPos = player_->GetWorldTransform()->GetWorldPosition();
	Vector3 myPos = worldTransform_.GetWorldPosition();

	// 3次元空間でプレイヤーとの距離を計算
	Vector3 toPlayer = playerPos - myPos;
	float length = Length(toPlayer);

	// 一定の索敵範囲に入ったら接近状態（Approach）へ
	if (length <= kApproachRange) {
		phase_ = Phase::kApproach;
	}
}

void RusherEnemy::UpdateApproach() {
	Vector3 playerPos = player_->GetWorldTransform()->GetWorldPosition();
	Vector3 myPos = worldTransform_.GetWorldPosition();

	// プレイヤーに向かう方向ベクトル
	Vector3 toPlayer = playerPos - myPos;
	float length = Length(toPlayer);

	if (length > 0.0f) {
		// 正規化した速度でゆっくり接近
		Vector3 move = Normalize(toPlayer) * kSpeed;
		worldTransform_.translation += move;
	}

	// 突撃範囲に入ったら突進状態（Rush）へ
	if (length <= kRushRange) {
		phase_ = Phase::kRush;
		// 突進する方向をロック
		if (length > 0.0f) {
			rushDirection_ = Normalize(toPlayer);
		} else {
			rushDirection_ = { 0.0f, 0.0f, -1.0f }; // デフォルトは手前（Z-）方向
		}
		rushTimer_ = kMaxRushTime;
	}
}

void RusherEnemy::UpdateRush() {
	// 突進方向に高速移動
	worldTransform_.translation += rushDirection_ * kRushSpeed;

	// タイマー更新
	rushTimer_ -= TimeManager::GetInstance()->GetDeltaTime();

	// 突進が終了したら待機状態へ
	if (rushTimer_ <= 0.0f) {
		phase_ = Phase::kIdle;
	}
}

void RusherEnemy::OnCollision() {
	// 基底クラスの衝突処理を呼び出し、生存フラグを落とす
	EnemyBase::OnCollision();

	// 衝突時の独自処理（例：ぶつかったら待機状態に戻す）
	phase_ = Phase::kIdle;
}
