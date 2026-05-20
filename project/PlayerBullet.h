#pragma once

#include <Vector3.h>

class Camera;
class Object3d;

class PlayerBullet {
public:
	void Initialize(Camera* camera, const Vector3& pos, Object3d* model);
	void Update();
	void Draw();

private:
	Camera* camera_ = nullptr;

	Object3d* model_ = nullptr;

	// 位置
	Vector3 pos_ = {0.0f, 0.0f, 0.0f};
};

