#pragma once

#include "IPlayerBullet.h"

class EnemyBase;

class HomingPlayerBullet : public IPlayerBullet {
public:
	HomingPlayerBullet() = default;
	~HomingPlayerBullet() override = default;

	HomingPlayerBullet(HomingPlayerBullet&&) noexcept = default;
	HomingPlayerBullet& operator=(HomingPlayerBullet&&) noexcept = default;

	// 基底クラスの仮想関数のオーバーライド
	void Initialize(const PlayerBulletParam& param) override;
	void Update(const std::list<EnemyBase*>& enemies) override;

	void SetTarget(EnemyBase* target) { target_ = target; }
	EnemyBase* GetTarget() const { return target_; }

private:
	EnemyBase* target_ = nullptr;
	
	// ホーミング弾は追尾時間を長くするため、寿命を長めに設定する（例: 2.0秒）
	static constexpr float kHomingLifeTime = 2.0f;
};
