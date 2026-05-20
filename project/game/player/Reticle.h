#pragma once

#include <Vector3.h>

#include <memory>

class Camera;
class Plane;

class Reticle {
public:
	Reticle();
	~Reticle();
public:
	void Initialize(Camera* camera, const Vector3& pos);
	void Update();
	void Draw();

	void SetPosition(const Vector3& playerPos);

private:
	// カメラ
	Camera* camera_ = nullptr;

	// 板ポリ
	std::unique_ptr<Plane> plane_;

	// サイズ
	Vector3 scale_ = {0.5f, 0.5f, 0.5f};
	// 位置
	Vector3 pos_ = {0.0f, 0.0f, 0.0f};
};

