#pragma once

#include "IPlayerBullet.h"

class NormalPlayerBullet : public IPlayerBullet {
public:
	NormalPlayerBullet() = default;
	~NormalPlayerBullet() override = default;

	NormalPlayerBullet(NormalPlayerBullet&&) noexcept = default;
	NormalPlayerBullet& operator=(NormalPlayerBullet&&) noexcept = default;

	void Initialize(const PlayerBulletParam& param) override;

	void Update(const std::list<EnemyBase*>& enemies) override;

private:
	// 寿命
	static constexpr float kLifeTime = 0.2f;
};
