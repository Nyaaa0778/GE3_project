#pragma once

#include <Vector3.h>

class Camera;
class Object3d;
class PlayerBulletPool;

class PlayerBullet {
public:
	~PlayerBullet();
	void Initialize(Camera* camera, const Vector3& pos, const Vector3& velocity, PlayerBulletPool* bulletPool);
	void Update();
	void Draw();

	bool IsDead() const { return isDead_; }

	// コリジョン用
	const Vector3& GetPosition() const { return pos_; }
	Vector3 GetCollisionSize() const { return kCollisionSize; }
	void OnCollision() { isDead_ = true; }

private:
	Camera* camera_ = nullptr;

	Object3d* model_ = nullptr;

	PlayerBulletPool* bulletPool_ = nullptr;

	// 位置
	Vector3 pos_ = {0.0f, 0.0f, 0.0f};
	// 速度
	Vector3 velocity_ = {0.0f, 0.0f, 0.0f};

	// 当たり判定のサイズ
	static inline const Vector3 kCollisionSize = {0.5f, 0.5f, 0.5f};

	// 寿命
	static const int kLifeTime = 60 * 2;
	int deathTimer_ = kLifeTime;
	// 消滅フラグ
	bool isDead_ = false;

private:
	void UpdateMove();
};

