#pragma once

#include <memory>

#include <Vector3.h>
#include <WorldTransform.h>

class Object3d;
class Camera;

class Player {
public:
	void Initialize(const Vector3& InitialPos, Object3d* model, Camera* camera);
	void Update();
	void Draw();

	const WorldTransform* GetWorldTransform() const { return &worldTransform_; }
	WorldTransform* GetWorldTransform() { return &worldTransform_; }

private:
	// モデル
	Object3d* model_ = nullptr;

	// トランスフォーム
	WorldTransform worldTransform_;
	// 当たり判定のサイズ
	Vector3 kCollisionSize_ = {1.0f, 1.0f, 1.0f};

	// 速さ
	float kBaseSpeed = 0.3f;
	// 速度
	Vector3 velocity_ = {0.0f, 0.0f, 0.0f};

	// 移動制限
	static constexpr float kMoveLimitX = 7.0f;
	static constexpr float kMoveLimitY = 3.5f;

	// 生存フラグ
	bool isAlive_ = true;

private:
	/// <summary>
	/// 移動処理
	/// </summary>
	void UpdateMove();
};

