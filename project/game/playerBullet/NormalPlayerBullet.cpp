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
	worldTransform_.scale = {0.5f, 0.5f, 0.5f};
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
	prevWorldPos_ = worldTransform_.GetWorldPosition();

	worldTransform_.translation += velocity_;
	worldTransform_.UpdateMatrix();
	model_->Update(&worldTransform_);

	if (!isDead_) {
		deathTimer_ -= TimeManager::GetInstance()->GetDeltaTime();

		if (deathTimer_ <= 0.0f) {
			isDead_ = true;
		}
	}
}
