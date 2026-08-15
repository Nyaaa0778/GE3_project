#include "HomingPlayerBullet.h"

#include <MathUtility.h>
#include <MyEngine.h>
#include "EnemyBase.h"

using namespace MathUtility;

void HomingPlayerBullet::Initialize(const PlayerBulletParam& param) {
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
	
	target_ = param.target;
	deathTimer_ = kHomingLifeTime;
	isDead_ = false;
}

void HomingPlayerBullet::Update(const std::list<EnemyBase*>& enemies) {
	// 移動前のワールド座標を保存
	prevWorldPos_ = worldTransform_.GetWorldPosition();

	// ターゲットの生存確認
	if (target_) {
		// ターゲットがまだ生存している（リストに存在する）か確認
		auto it = std::find(enemies.begin(), enemies.end(), target_);
		if (it == enemies.end()) {
			target_ = nullptr;
		}
	}

	if (target_) {
		// ターゲットの位置を取得
		Vector3 targetPos = target_->GetWorldPosition();
		
		// ターゲットへの方向ベクトルを算出し、正規化する
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
