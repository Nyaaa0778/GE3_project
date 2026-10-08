#pragma once

#include <memory>
#include <vector>
#include <list>

#include <Vector2.h>
#include <Vector3.h>
#include <WorldTransform.h>

#include "Collider.h"
#include "Reticle.h"
#include "ObjectPool.h"
#include "NormalPlayerBullet.h"
#include "HomingPlayerBullet.h"

class Object3d;
class Camera;
class Primitive;
class Sprite;
class WireframeObject;

class LockOn;
class IPlayerState;

class EnemyBase;

class Player : public Collider {
public:

	Player();
	~Player();

	void Initialize(const Vector3& InitialPos, Object3d* model, Camera* camera);
	
	void Update(const std::list<EnemyBase*>& enemies);
	
	// アニメーション・トランスフォームのみ更新（クリア演出用）
	void UpdateAnimationOnly();
	
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
	const std::list<IPlayerBullet*>& GetBullets() const { return bullets_; }

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

	std::list<IPlayerBullet*> bullets_;

	ObjectPool<NormalPlayerBullet> normalBulletPool_;
	ObjectPool<HomingPlayerBullet> homingBulletPool_;

	static constexpr size_t kMaxBullets = 50;

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

	// ------------------------------------
	// 数式制御多面体ビジュアル専用関数
	// ------------------------------------
	void InitializeGeometricVisual();
	void UpdateGeometricVisual(float deltaTime);
	void DrawGeometricVisual();

	// 幾何学多面体パーツ（数式描画プレイヤー）
	// 外殻：正二十面体 (Icosahedron)
	std::unique_ptr<WireframeObject> outerShell_;
	// 中間層：正十二面体 (Dodecahedron)
	std::unique_ptr<WireframeObject> middleShell_;
	// ジャイロリング
	std::unique_ptr<WireframeObject> gyroRingX_;
	std::unique_ptr<WireframeObject> gyroRingY_;
	// 内側中心核（高輝度当たり判定ビーコン）
	std::unique_ptr<WireframeObject> centerCore_;
	// 内側偏心公転光核：シアン光
	std::unique_ptr<WireframeObject> cyanCore_;
	// 内側偏心公転光核：マゼンタ光
	std::unique_ptr<WireframeObject> magentaCore_;
	// 前方幾何学ポインター（進行方向・照準用）
	std::unique_ptr<WireframeObject> forwardPointer_;

	// 各パーツの回転・アニメーション用パラメータ
	Vector3 outerRotation_ = {0.0f, 0.0f, 0.0f};
	Vector3 middleRotation_ = {0.0f, 0.0f, 0.0f};
	Vector3 gyroRotationX_ = {0.0f, 0.0f, 0.0f};
	Vector3 gyroRotationY_ = {0.0f, 0.0f, 0.0f};
	Vector3 centerCoreRotation_ = {0.0f, 0.0f, 0.0f};

	// 操作追従バンク・ティルト角
	Vector3 currentTilt_ = {0.0f, 0.0f, 0.0f};
	Vector3 targetTilt_ = {0.0f, 0.0f, 0.0f};

	// ビジュアル用経過時間タイマー
	float visualTime_ = 0.0f;
};

