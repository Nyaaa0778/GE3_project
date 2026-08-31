#pragma once

#include <Vector3.h>

#include "WorldTransform.h"
#include "Collider.h"

class Object3d;
class Camera;

class EnemyBase : public Collider {
public:
	enum class EnemyState {
		kAlive, // 通常生存（行動中・当たり判定あり）
		kDying, // 死亡演出中（グリッチ中・当たり判定なし）
		kDead,  // 死亡完了（破棄待ち）
	};

public:
	virtual ~EnemyBase() = default;

	virtual void Initialize(Object3d* model, Camera* camera, const Vector3& pos);
	virtual void Update();
	virtual void Draw();

	// コライダーの仮想関数をオーバーライド
	virtual void OnCollision() override;
	virtual Vector3 GetWorldPosition() override;

public:
	const WorldTransform& GetWorldTransform() const { return worldTransform_; }
	WorldTransform& GetWorldTransform() { return worldTransform_; }

	// 状態確認
	bool IsAlive() const { return state_ == EnemyState::kAlive; }
	bool IsDying() const { return state_ == EnemyState::kDying; }
	bool IsDead() const { return state_ == EnemyState::kDead; }

	// スコア付与済みフラグ
	bool HasGivenScore() const { return hasGivenScore_; }
	void SetScoreGiven(bool given) { hasGivenScore_ = given; }

	// スコア値取得
	virtual int GetScore() const { return 100; }

protected:
	// 死亡演出（グリッチ・パーティクル）の更新
	virtual void UpdateDeath();

protected:
	// ワールド変換データ
	WorldTransform worldTransform_;

	// モデル
	Object3d* model_ = nullptr;

	// カメラ
	Camera* camera_ = nullptr;

	// 状態
	EnemyState state_ = EnemyState::kAlive;
	bool hasGivenScore_ = false;

	// 死亡演出用
	float deathTimer_ = 0.0f;
	static constexpr float kDeathDuration = 0.35f;
	Vector3 deathBasePos_ = {};
	Vector3 baseScale_ = {1.0f, 1.0f, 1.0f};
	int glitchFrameCount_ = 0;
};


