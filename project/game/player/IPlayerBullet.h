#pragma once

#include <memory>
#include <list>

#include <Vector3.h>

#include "WorldTransform.h"
#include "Collider.h"

class Object3d;
class Camera;
class EnemyBase;

struct PlayerBulletParam {
	Camera* camera = nullptr;
	Vector3 position = {};
	Vector3 velocity = {};
	EnemyBase* target = nullptr;
};

class IPlayerBullet : public Collider {
public:
	// コライダーの仮想関数をオーバーライド
	void OnCollision() override;
	Vector3 GetWorldPosition() override;
	Vector3 GetPrevWorldPosition() override;

public:
	virtual ~IPlayerBullet() override = default;

	virtual void Initialize(const PlayerBulletParam& param) = 0;

	virtual void Update(const std::list<EnemyBase*>& enemies) = 0;

	void Draw();

	Vector3 GetPosition() const { return worldTransform_.translation; }

	bool IsDead() const { return isDead_; }

protected:
	std::unique_ptr<Object3d> model_;
	WorldTransform worldTransform_;
	Vector3 velocity_ = {};
	Vector3 prevWorldPos_ = {};

	// 消滅タイマー
	float deathTimer_ = 0.0f;
	// 消滅フラグ
	bool isDead_ = false;
};
