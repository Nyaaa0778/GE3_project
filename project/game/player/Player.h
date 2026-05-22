#pragma once

#include <Vector3.h>

#include <memory>
#include <list>
#include <string>

class Camera;
class Object3d;
class Reticle;
class PlayerBullet;
class PlayerBulletPool;

class Player {
public:
	Player();
	~Player();
public:
	void Initialize(Camera* camera, const Vector3& pos, Object3d* model, const std::string& bulletModelName);

	// 位置の取得
	const Vector3& GetPos() const { return pos_; }

	void Update();

	void Draw();

private:
	// カメラ
	Camera* camera_ = nullptr;

	// モデル
	Object3d* model_ = nullptr;

	// 位置
	Vector3 pos_ = {0.0f, 0.0f, 0.0f};
	// 速さ
	static inline const float kBaseSpeed = 0.1f;

	// 移動制限
	static inline const Vector3 kMoveLimit = {7.0f, 3.5f, 0.0f};

	// 当たり判定の大きさ
	static inline const Vector3 collisionSize_ = {1.0f, 1.0f, 1.0f};

	// -----------------------
	// 照準
	// -----------------------

	std::unique_ptr<Reticle> reticle_;

	// 奥行固定
	static inline const float kDepthPos = 5.0f;

	// 位置
	Vector3 reticlePos_ = {0.0f, 0.0f, kDepthPos};

	// -----------------------
	// 弾（複数弾）
	// -----------------------

	std::list<std::unique_ptr<PlayerBullet>> bullets_;

	std::unique_ptr<PlayerBulletPool> bulletPool_;

	// 位置
	Vector3 bulletPos_ = {0.0f, 0.0f, 0.0f};

	// 発射のクールタイム（秒単位）。0.25fなら1秒間に4発
	static inline const float kBulletCooldown = 0.15f;
	// クールタイムを計測するタイマー（秒）
	float bulletCooldownTimer_ = 0.0f;

private:
	/// <summary>
	/// 移動処理
	/// </summary>
	void UpdateMove();

	void UpdateReticle();

	/// <summary>
	/// 弾の移動処理
	/// </summary>
	void UpdateBullets();

	/// <summary>
	/// ImGuiの描画
	/// </summary>
	void UpdateImGui();
};

