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
	Vector3 startPos_ = {};
	Vector3 targetPos_ = {};
	Vector3 forwardDir_ = { 0.0f, 0.0f, 1.0f };
	Vector3 sideDir_ = { 1.0f, 0.0f, 0.0f };
	float curveSignX_ = 1.0f;
	float arcWidth_ = 5.0f;
	float progress_ = 0.0f;
	float progressSpeed_ = 2.0f;
	bool hasArrived_ = false;
};
