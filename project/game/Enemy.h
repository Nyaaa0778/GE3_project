#pragma once
#include "Object3d.h"
#include "Vector3.h"
#include <memory>
#include <random>

class Camera;

class Enemy {
public:
	enum class State {
		Wander,
		Wait
	};

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

	int GetWaitTime() const { return waitTimeMax_; }
	void SetWaitTime(int waitTime) { waitTimeMax_ = waitTime; }

	int GetSeed() const { return seed_; }
	void SetSeed(int seed);

	State GetState() const { return state_; }
	float GetHp() const { return hp_; }
	void SetHp(float hp) { hp_ = hp; }

private:
	std::unique_ptr<Object3d> obj_;
	Vector3 position_ = { 0.0f, 0.0f, 0.0f };
	Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };
	float speed_ = 0.05f;
	float hp_ = 100.0f;

	State state_ = State::Wander;
	int timer_ = 0;
	int waitTimeMax_ = 60; // 待機時間（フレーム数）
	int seed_ = 42;
	std::mt19937 randGen_;
	Vector3 wanderTarget_ = { 0.0f, 0.0f, 0.0f };

	void ChooseNewTarget();
};
