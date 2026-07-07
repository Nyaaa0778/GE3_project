#include "HomingPlayerBullet.h"

#include <MathUtility.h>
#include <MyEngine.h>
#include "EnemyBase.h"

using namespace MathUtility;

void HomingPlayerBullet::Initialize(Camera* camera, const Vector3& pos, const Vector3& velocity, EnemyBase* target) {
	PlayerBullet::Initialize(camera, pos, velocity);
	target_ = target;
	
	// 寿命をホーミング弾用に上書き
	deathTimer_ = kHomingLifeTime;
}

void HomingPlayerBullet::Update() {
	// 移動前のワールド座標を保存
	prevWorldPos_ = worldTransform_.GetWorldPosition();

	if (target_) {
		// ターゲットの位置を取得
		Vector3 targetPos = target_->GetWorldPosition();
		
		// LockOnで行っているのと同様に、ターゲットへの方向ベクトルを算出し、正規化する
		Vector3 shootDir = targetPos - worldTransform_.translation;
		float dist = Length(shootDir);
		if (dist > 0.001f) {
			shootDir = Normalize(shootDir);
			
			// 弾速（元の速度ベクトルの大きさ）を保ったままターゲット方向へ速度を更新
			float speed = Length(velocity_);
			velocity_ = shootDir * speed;
		}
	}

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
