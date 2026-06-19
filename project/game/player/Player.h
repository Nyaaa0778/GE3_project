#pragma once

#include <memory>
#include <vector>
#include <list>

#include <Vector3.h>
#include <WorldTransform.h>

#include "Collider.h"

class Object3d;
class Camera;
class Primitive;

class PlayerBullet;

class Player : public Collider {
public:
	Player();
	~Player();
	void Initialize(const Vector3& InitialPos, Object3d* model, Camera* camera);
	void Update();
	void Draw();

	// コライダーの仮想関数をオーバーライド
	void OnCollision() override;
	Vector3 GetWorldPosition() override;


public:
	const WorldTransform* GetWorldTransform() const { return &worldTransform_; }
	WorldTransform* GetWorldTransform() { return &worldTransform_; }

	decltype(auto) GetReticleMatWorld() const { return worldTransformReticle_.matWorld; }
	bool GetIsReticleHit() const { return isReticleHit_; }

	void SetParent(const WorldTransform* parent) { worldTransform_.parent = parent; }

	// 弾リストのゲッター
	const std::list<std::unique_ptr<PlayerBullet>>& GetBullets() const { return bullets_; }
private:
	// ------------------------------------
	// 本体
	// ------------------------------------
	
	// モデル
	Object3d* model_ = nullptr;

	// トランスフォーム
	WorldTransform worldTransform_;

	// 3Dレティクルのワールド座標 - 自機のワールド座標
	Vector3 reticleWorldPos = {};

	// 速さ
	float kBaseSpeed = 0.3f;
	// 速度
	Vector3 velocity_ = {0.0f, 0.0f, 0.0f};

	// 移動制限
	static constexpr float kMoveLimitX = 7.0f;
	static constexpr float kMoveLimitY = 3.5f;

	// 生存フラグ
	bool isAlive_ = true;
	bool isReticleHit_ = false;

	// 当たり判定の大きさ
	static constexpr Vector3 kCollisionSize = {1.0f, 1.0f, 1.0f};

	// ------------------------------------
	// 照準
	// ------------------------------------

	WorldTransform worldTransformReticle_;

	// モデル
	std::unique_ptr<Object3d> reticle_;
	// 描画サイズ
	static constexpr Vector3 kReticleDrawSize = {0.5f, 0.5f, 0.5f};

	// ------------------------------------
	// カメラ
	// ------------------------------------

	Camera* camera_ = nullptr;

	// ------------------------------------
	// 弾
	// ------------------------------------

	std::list<std::unique_ptr<PlayerBullet>> bullets_;

	Vector3 bulletVelocity_ = {0.0f, 0.0f, 0.0f};
	static constexpr float kBulletSpeed = 2.0f;

	static constexpr float kCooldownDuration = 0.1f;
	float cooldownTimer_ = 0.0f;

private:
	/// <summary>
	/// 移動処理
	/// </summary>
	void UpdateMove();

	/// <summary>
	/// 照準の更新
	/// </summary>
	void UpdateReticle();

	/// <summary>
	/// 弾の更新
	/// </summary>
	void UpdateBullet();

	void Attack();
};

