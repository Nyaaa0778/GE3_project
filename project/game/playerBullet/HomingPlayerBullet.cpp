#include "HomingPlayerBullet.h"

#include <MathUtility.h>
#include <MyEngine.h>
#include "EnemyBase.h"
#include "Random.h"

#include <numbers>
#include <cmath>
#include <algorithm>

using namespace MathUtility;

void HomingPlayerBullet::Initialize(const PlayerBulletParam& param) {
	if (!model_) {
		model_ = std::make_unique<Object3d>();
		model_->Initialize("sphere");
	}
	model_->SetCamera(param.camera);

	worldTransform_.Initialize();
	worldTransform_.translation = param.position;
	worldTransform_.scale = {0.0f, 0.0f, 0.0f};

	startPos_ = param.position;
	prevWorldPos_ = param.position;
	target_ = param.target;
	if (target_) {
		targetPos_ = target_->GetWorldPosition();
	} else {
		Vector3 forwardDir = Normalize(param.velocity);
		if (Length(forwardDir) < 0.001f) {
			forwardDir = { 0.0f, 0.0f, 1.0f };
		}
		targetPos_ = param.position + forwardDir * 100.0f;
	}

	// X軸方向に左右ランダムに弧を描く符号（+1 または -1）
	curveSignX_ = (Random::RangeFloat(0.0f, 1.0f) > 0.5f) ? 1.0f : -1.0f;

	float dist = Length(targetPos_ - startPos_);
	// 距離に応じて弧の幅を動的に設定
	arcWidth_ = std::clamp(dist * 0.25f, 3.0f, 10.0f) * Random::RangeFloat(0.9f, 1.1f);

	// 進行速度（0.4秒〜1.0秒程度で着弾）
	float duration = std::clamp(dist / 90.0f, 0.4f, 1.0f);
	progress_ = 0.0f;
	progressSpeed_ = 1.0f / duration;

	// コライダーの初期設定 (球, 半径 0.5)
	SetShape(ColliderShape::kSphere);
	SetSphere({ 0.5f });

	isDead_ = false;
}

void HomingPlayerBullet::Update(const std::list<EnemyBase*>& enemies) {
	// 移動前のワールド座標を保存
	prevWorldPos_ = worldTransform_.GetWorldPosition();

	// ターゲットの生存確認
	if (target_) {
		auto it = std::find(enemies.begin(), enemies.end(), target_);
		if (it == enemies.end()) {
			target_ = nullptr;
		} else {
			// 生存していればターゲットの最新座標を取得
			targetPos_ = target_->GetWorldPosition();
		}
	}

	float dt = TimeManager::GetInstance()->GetDeltaTime();
	progress_ += progressSpeed_ * dt;

	if (progress_ >= 1.0f) {
		progress_ = 1.0f;
		isDead_ = true;
	}

	// 始点からターゲット座標への直線補間
	Vector3 basePos = startPos_ + (targetPos_ - startPos_) * progress_;

	// X軸方向にサイン波の弧 (sin(p * π)) を加算
	float arc = std::sin(progress_ * std::numbers::pi_v<float>) * arcWidth_ * curveSignX_;
	worldTransform_.translation = { basePos.x + arc, basePos.y, basePos.z };
	worldTransform_.UpdateMatrix();
	model_->Update(&worldTransform_);

	// 光の軌道（パーティクルのトレイル）を生成
	Vector3 startStep = prevWorldPos_;
	Vector3 endStep = worldTransform_.GetWorldPosition();
	Vector3 diff = endStep - startStep;
	float totalDist = Length(diff);
	float stepDist = 0.2f;
	int numSteps = static_cast<int>(totalDist / stepDist);
	if (numSteps < 1) {
		numSteps = 1;
	}

	for (int i = 0; i < numSteps; ++i) {
		float t = static_cast<float>(i) / static_cast<float>(numSteps);
		Vector3 trailPos = startStep + diff * t;
		Vector4 trailColor = { 1.0f, 0.3f, 0.85f, 1.0f }; // 鮮やかなピンク・マゼンタ
		Vector3 trailScale = { 0.45f, 0.45f, 0.45f };
		float trailLifeTime = 0.25f;
		ParticleManager::GetInstance()->Emit("CircleParticle", trailPos, {0.0f, 0.0f, 0.0f}, trailColor, trailScale, trailLifeTime, 1);
	}

	// 弾頭のコア（ピンクがかった白い高輝度パーティクル）
	Vector4 coreColor = { 1.0f, 0.9f, 0.95f, 1.0f };
	Vector3 coreScale = { 0.65f, 0.65f, 0.65f };
	float coreLifeTime = 0.05f;
	ParticleManager::GetInstance()->Emit("CircleParticle", endStep, {0.0f, 0.0f, 0.0f}, coreColor, coreScale, coreLifeTime, 1);
}
