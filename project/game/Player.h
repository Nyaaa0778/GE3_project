#pragma once
#include "Object3d.h"
#include "Vector3.h"
#include <memory>

class Camera;

class Player {
public:
	void Initialize(Camera* camera, const Vector3& initialPosition);
	void Update(bool isPaused);
	void Draw();

	// Getters / Setters
	const Vector3& GetPosition() const { return position_; }
	void SetPosition(const Vector3& pos);

	const Vector3& GetRotation() const { return rotation_; }
	void SetRotation(const Vector3& rot);

	float GetSpeed() const { return speed_; }
	void SetSpeed(float speed) { speed_ = speed; }

private:
	std::unique_ptr<Object3d> obj_;
	Vector3 position_ = { 0.0f, 0.0f, 0.0f };
	Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };
	float speed_ = 0.1f;
};
