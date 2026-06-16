#pragma once

#include <memory>

#include <Vector3.h>

#include "WorldTransform.h"

class Object3d;
class Camera;

class PlayerBullet {
public:
	void Initialize(Camera* camera, const Vector3& pos, const Vector3& velocity);

	void Update();

	void Draw();

	Vector3 GetPosition() const { return worldTransform_.translation; }

private:
	std::unique_ptr<Object3d> model_;
	WorldTransform worldTransform_;
	Vector3 velocity_ = {};

	// 寿命
	static constexpr float kLifeTime = 120.0f;
	// 消滅タイマー
	float deathTimer_ = kLifeTime;
	// 消滅フラグ
	bool isDead_ = false;
};

