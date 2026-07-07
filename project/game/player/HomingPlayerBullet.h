#pragma once

#include "PlayerBullet.h"

class EnemyBase;

class HomingPlayerBullet : public PlayerBullet {
public:
	void Initialize(Camera* camera, const Vector3& pos, const Vector3& velocity, EnemyBase* target);

	void Update() override;

	// ホーミング用の仮想インターフェースの実装
	void SetTarget(EnemyBase* target) override { target_ = target; }
	EnemyBase* GetTarget() const override { return target_; }

private:
	EnemyBase* target_ = nullptr;
	
	// ホーミング弾は追尾時間を長くするため、寿命を長めに設定する（例: 2.0秒）
	static constexpr float kHomingLifeTime = 2.0f;
};
