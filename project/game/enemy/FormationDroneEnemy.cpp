#include "FormationDroneEnemy.h"

#include <MathUtility.h>
#include <TimeManager.h>
#include <cmath>

using namespace MathUtility;

void FormationDroneEnemy::Initialize(Object3d* model, Camera* camera, const Vector3& basePos, const FormationConfig& config) {
	// 基底クラスの初期化
	EnemyBase::Initialize(model, camera, basePos);

	config_ = config;
	basePos_ = basePos;
	elapsedTime_ = 0.0f;

	// コライダーの初期設定 (AABB, 小型)
	SetShape(ColliderShape::kAABB);
	SetAABB({ kCollisionSize });

	// ドローン用の小型スケール
	worldTransform_.scale = { 0.6f, 0.6f, 0.6f };

	// 初期位置の設定
	worldTransform_.translation = CalculatePosition(0.0f);
	worldTransform_.UpdateMatrix();
}

void FormationDroneEnemy::Update() {
	if (!IsAlive()) {
		EnemyBase::Update();
		return;
	}

	float dt = TimeManager::GetInstance()->GetDeltaTime();
	elapsedTime_ += dt;

	// 生存時間を超えたら自動退場（自然消滅）
	if (elapsedTime_ >= config_.lifeTime) {
		isAlive_ = false;
		return;
	}

	// 現在位置の保存
	Vector3 prevPos = worldTransform_.translation;

	// 新しい位置の計算と適用
	Vector3 newPos = CalculatePosition(elapsedTime_);
	worldTransform_.translation = newPos;

	// 移動量ベクトルから姿勢（向き・傾き）を計算
	Vector3 velocity = newPos - prevPos;
	UpdateRotation(velocity);

	// 基底クラスの更新（行列更新・モデル更新）
	EnemyBase::Update();
}

void FormationDroneEnemy::OnCollision() {
	// 撃破
	EnemyBase::OnCollision();
}

Vector3 FormationDroneEnemy::CalculatePosition(float t) {
	Vector3 pos = basePos_ + config_.localOffset;

	switch (config_.pattern) {
	case DroneFlightPattern::kSineWave: {
		// 手前へ前進しつつ、上下・左右にサイン波ウェーブ
		Vector3 forward = config_.moveDirection * (config_.speed * t);
		float waveY = std::sin(t * config_.waveFrequency + config_.phaseOffset) * config_.waveAmplitude;
		float waveX = std::cos(t * config_.waveFrequency * 0.5f + config_.phaseOffset) * (config_.waveAmplitude * 0.4f);
		pos += forward + Vector3{ waveX, waveY, 0.0f };
		break;
	}
	case DroneFlightPattern::kCircle: {
		// 基準位置の周りを旋回しながら前進
		float angle = t * config_.waveFrequency + config_.phaseOffset;
		Vector3 circleOffset = {
			std::cos(angle) * config_.waveAmplitude,
			std::sin(angle) * config_.waveAmplitude,
			0.0f
		};
		Vector3 forward = config_.moveDirection * (config_.speed * 0.6f * t);
		pos += forward + circleOffset;
		break;
	}
	case DroneFlightPattern::kSlalom: {
		// 左右に大きくS字蛇行しながら前進
		Vector3 forward = config_.moveDirection * (config_.speed * t);
		float waveX = std::sin(t * config_.waveFrequency + config_.phaseOffset) * (config_.waveAmplitude * 1.6f);
		pos += forward + Vector3{ waveX, 0.0f, 0.0f };
		break;
	}
	case DroneFlightPattern::kFigureEight: {
		// 8の字旋回を描きながら前進
		float angle = t * config_.waveFrequency + config_.phaseOffset;
		float waveX = std::sin(angle) * config_.waveAmplitude;
		float waveY = std::sin(angle * 2.0f) * (config_.waveAmplitude * 0.5f);
		Vector3 forward = config_.moveDirection * (config_.speed * 0.7f * t);
		pos += forward + Vector3{ waveX, waveY, 0.0f };
		break;
	}
	}

	return pos;
}

void FormationDroneEnemy::UpdateRotation(const Vector3& velocity) {
	float len = Length(velocity);
	if (len > 0.0001f) {
		// 進行方向に向ける（ヨー・ピッチ）
		float yaw = std::atan2(velocity.x, velocity.z);
		float horizontalLen = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
		float pitch = std::atan2(-velocity.y, horizontalLen);

		// 左右旋回に応じたバンク角（ロール）を付加
		float roll = -velocity.x * 2.0f;

		worldTransform_.rotation = { pitch, yaw, roll };
	}
}
