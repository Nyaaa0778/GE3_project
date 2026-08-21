#include "NormalPlayerBullet.h"

#include <cassert>

#include <MyEngine.h>
#include <MathUtility.h>

using namespace MathUtility;

void NormalPlayerBullet::Initialize(const PlayerBulletParam& param) {
	if (!model_) {
		model_ = std::make_unique<Object3d>();
		model_->Initialize("sphere");
	}
	model_->SetCamera(param.camera);

	worldTransform_.Initialize();
	worldTransform_.translation = param.position;
	worldTransform_.scale = {0.0f, 0.0f, 0.0f};
	worldTransform_.UpdateMatrix();
	velocity_ = param.velocity;

	// コライダーの初期設定 (球, 半径 0.2)
	SetShape(ColliderShape::kSphere);
	SetSphere({ 0.2f });

	prevWorldPos_ = param.position;
	deathTimer_ = kLifeTime;
	isDead_ = false;
}

void NormalPlayerBullet::Update(const std::list<EnemyBase*>& enemies) {
	(void)enemies; // 未使用警告防止

	// 移動前のワールド座標を保存
	prevWorldPos_ = worldTransform_.translation;

	worldTransform_.translation += velocity_;
	worldTransform_.UpdateMatrix();
	model_->Update(&worldTransform_);

	// 光の軌道（パーティクルのトレイル）を生成
	Vector3 startPos = prevWorldPos_;
	Vector3 endPos = worldTransform_.translation;
	Vector3 diff = endPos - startPos;
	float totalDist = Length(diff);
	float stepDist = 0.25f;
	int numSteps = static_cast<int>(totalDist / stepDist);
	if (numSteps < 1) {
		numSteps = 1;
	}

	for (int i = 0; i < numSteps; ++i) {
		float t = static_cast<float>(i) / static_cast<float>(numSteps);
		Vector3 trailPos = startPos + diff * t;
		Vector4 trailColor = { 0.2f, 0.75f, 1.0f, 1.0f }; // 鮮やかなシアン
		Vector3 trailScale = { 0.45f, 0.45f, 0.45f };
		float trailLifeTime = 0.07f; // 短寿命で残像が自機後方に残らないようにする
		ParticleManager::GetInstance()->Emit("BulletTrail", trailPos, {0.0f, 0.0f, 0.0f}, trailColor, trailScale, trailLifeTime, 1);
	}

	// 弾頭のコア（白い高輝度パーティクル）
	Vector4 coreColor = { 0.8f, 0.95f, 1.0f, 1.0f };
	Vector3 coreScale = { 0.65f, 0.65f, 0.65f };
	float coreLifeTime = 0.05f;
	ParticleManager::GetInstance()->Emit("BulletTrail", endPos, {0.0f, 0.0f, 0.0f}, coreColor, coreScale, coreLifeTime, 1);

	if (!isDead_) {
		deathTimer_ -= TimeManager::GetInstance()->GetDeltaTime();

		if (deathTimer_ <= 0.0f) {
			isDead_ = true;
		}
	}
}
