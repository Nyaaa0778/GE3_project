#pragma once

#include <memory>

#include <Vector3.h>

#include "WorldTransform.h"

class Object3d;
class Camera;

class PlayerBullet {
public:
	void Initialize(Camera* camera, const Vector3& pos, const Vector3& velocity, const WorldTransform* parent = nullptr);

	void Update();

	void Draw();

	Vector3 GetPosition() const { return worldTransform_.translation; }

private:
	std::unique_ptr<Object3d> model_; // 借り物をやめて所有する
	WorldTransform worldTransform_;
	Vector3 velocity_ = {};
};

