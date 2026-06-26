#pragma once

#include <memory>

#include <Vector3.h>

#include "WorldTransform.h"
#include "Collider.h"

class Object3d;
class Camera;

class PlayerBullet : public Collider {
public:
	// コライダーの仮想関数をオーバーライド
	void OnCollision() override;
	Vector3 GetWorldPosition() override;
	Vector3 GetPrevWorldPosition() override;
public:
	void Initialize(Camera* camera, const Vector3& pos, const Vector3& velocity);

	void Update();

	void Draw();

	Vector3 GetPosition() const { return worldTransform_.translation; }

	bool IsDead() const { return isDead_; }
private:
	std::unique_ptr<Object3d> model_;
	WorldTransform worldTransform_;
	Vector3 velocity_ = {};
	Vector3 prevWorldPos_ = {};

	// 寿命
	static constexpr float kLifeTime = 0.2f;
	// 消滅タイマー
	float deathTimer_ = kLifeTime;
	// 消滅フラグ
	bool isDead_ = false;
};
