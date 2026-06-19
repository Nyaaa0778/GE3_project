#pragma once

#include "Collider.h"
#include "WorldTransform.h"
#include "Cylinder.h"
#include <memory>

class Camera;

class Goal : public Collider {
public:
	Goal();
	~Goal();

	void Initialize(const Vector3& pos, Camera* camera);
	void Update();
	void Draw();
	void DrawImGui(const char* windowName);

	// コライダーの仮想関数をオーバーライド
	void OnCollision() override;
	Vector3 GetWorldPosition() override;

public:
	const WorldTransform& GetWorldTransform() const { return worldTransform_; }
	WorldTransform& GetWorldTransform() { return worldTransform_; }

private:
	// ワールド変換データ
	WorldTransform worldTransform_;

	// ワープゾーンの見た目となる円柱
	std::unique_ptr<Cylinder> cylinder_;
	Camera* camera_ = nullptr;

	// 回転・UVスクロールアニメーション用
	Vector3 basePosition_ = {};
	Vector2 uvTranslation_ = {};

	float colorTimer_ = 0.0f;
	Vector4 color_ = {1.0f, 1.0f, 1.0f, 1.0f}; // RGBA
};
