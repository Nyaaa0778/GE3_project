#pragma once

#include <memory>
#include <vector>
#include <list>

#include <Vector2.h>
#include <Vector3.h>
#include <WorldTransform.h>

#include "Collider.h"
#include "Reticle.h"

class Object3d;
class Camera;
class Primitive;
class Sprite;

class LockOn;
class IPlayerBullet;
class IPlayerState;

class EnemyBase;

class Player : public Collider {
public:

	Player();
	~Player();

	void Initialize(const Vector3& InitialPos, Object3d* model, Camera* camera);
	
	void Update(const std::list<EnemyBase*>& enemies);
	
	void Draw();

	// コライダーの仮想関数をオーバーライド
	void OnCollision() override;
	Vector3 GetWorldPosition() override;

	/// <summary>
	/// ステートチェンジ
	/// </summary>
	void ChangeState(std::unique_ptr<IPlayerState> newState);

public:
	const WorldTransform* GetWorldTransform() const { return &worldTransform_; }
	WorldTransform* GetWorldTransform() { return &worldTransform_; }

	const Matrix4x4& GetReticleMatWorld() const { return reticle_->GetMatWorld(); }
	Vector2 GetReticle2DPosition() const { return reticle_->Get2DPosition(); }
	bool GetIsReticleHit() const { return isReticleHit_; }

	void SetParent(const WorldTransform* parent) { worldTransform_.parent = parent; }

	//ロックオンをセット
	void SetLockOn(LockOn* lockOn) { lockOn_ = lockOn; }

	// ロックオンモードのゲッター
	bool GetIsLockOnMode() const { return isLockOnMode_; }

	// 弾リストのゲッター
	const std::list<std::unique_ptr<IPlayerBullet>>& GetBullets() const { return bullets_; }

	// Dissolveをセット
	void SetDissolveEnable(bool useDissolve) { useDissolve_ = useDissolve; }
	void SetDissolveThreshold(float dissolveThreshold) { dissolveThreshold_ = dissolveThreshold; }
	float GetDissolveThreshold() const { return dissolveThreshold_; }

	// HPのゲッター・セッター
	float GetHP() const { return hp_; }
	void SetHP(float hp) { hp_ = hp; }

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

	// 現在のステート
	std::unique_ptr<IPlayerState> currentState_;

	// HP
	float hp_ = 100.0f;

	// 生存フラグ
	bool isAlive_ = true;
	bool isReticleHit_ = false;

	// ディゾルブ用パラメータ
	float dissolveThreshold_ = 0.0f;
	bool useDissolve_ = false;

public:
	// 生存フラグのゲッター
	bool IsAlive() const { return isAlive_; }

	// 当たり判定の大きさ
	static constexpr Vector3 kCollisionSize = {1.0f, 1.0f, 1.0f};

	// ディゾルブ時間（秒）
	static constexpr float kDissolveDuration = 1.5f;

	// ------------------------------------
	// 照準
	// ------------------------------------

	std::unique_ptr<Reticle> reticle_;

	// ------------------------------------
	// ロックオン
	// ------------------------------------

	LockOn* lockOn_ = nullptr;

	// ロックオンモードかどうかのフラグ（最初は通常モードなのでfalse）
	bool isLockOnMode_ = true;

	// ------------------------------------
	// カメラ
	// ------------------------------------

	Camera* camera_ = nullptr;

	// ------------------------------------
	// 弾
	// ------------------------------------

	std::list<std::unique_ptr<IPlayerBullet>> bullets_;

	Vector3 bulletVelocity_ = {0.0f, 0.0f, 0.0f};
	static constexpr float kBulletSpeed = 7.0f;

	static constexpr float kCooldownDuration = 0.1f;
	float cooldownTimer_ = 0.0f;

	// 前フレームのワールド座標（速度計算用）
	Vector3 prevWorldPos_ = { 0.0f, 0.0f, 0.0f };

private:

	/// <summary>
	/// 照準の更新
	/// </summary>
	void UpdateReticle();

	/// <summary>
	/// 弾の更新
	/// </summary>
	void UpdateBullet(const std::list<EnemyBase*>& enemies);

	/// <summary>
	/// 攻撃処理
	/// </summary>
	void Attack();
};

