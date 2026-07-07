#pragma once

#include <memory>
#include <Vector2.h>
#include <Vector3.h>
#include <WorldTransform.h>

class Object3d;
class Sprite;
class Camera;

class Reticle {
public:
	Reticle();
	~Reticle();

	// 照準の初期化
	void Initialize(Camera* camera);

	// 照準の更新（プレイヤーのワールドトランスフォームを受け取る）
	void Update(const WorldTransform& playerWorldTransform);

	// 照準の描画
	void Draw();

public:
	// ゲッター群
	const Matrix4x4& GetMatWorld() const { return worldTransformReticle_.matWorld; }
	Vector3 Get3DPosition() const { return worldTransformReticle_.GetWorldPosition(); }
	Vector2 Get2DPosition() const;

private:
	// 3Dレティクルのトランスフォーム
	WorldTransform worldTransformReticle_;

	// モデル
	std::unique_ptr<Object3d> reticle_;

	// スプライト
	std::unique_ptr<Sprite> reticleSprite_;

	// 描画サイズ
	static constexpr Vector3 kReticleDrawSize = {0.4f, 0.4f, 0.4f};

	Camera* camera_ = nullptr;
};
