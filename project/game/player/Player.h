#pragma once

#include <memory>
#include <list>

#include <Vector3.h>

#include "PlayerBullet.h"
#include "Plane.h"

class Object3d;
class Camera;

class Player {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="model">自機のモデル</param>
	/// <param name="pos">初期位置</param>
	void Initialize(Camera* camera, Object3d* model, Object3d* bulletModel, const Vector3& pos);
	/// <summary>
	/// 更新
	/// </summary>
	void Update();
	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

private:
	// カメラ
	Camera* camera_ = nullptr;

	// モデル
	Object3d* model_ = nullptr;

	// 位置
	Vector3 pos_ = {0.0f, 0.0f, 0.0f};
	// 速さ
	static inline const float kBaseSpeed = 0.2f;

	// 当たり判定の大きさ
	static inline const Vector3 collisionSize_ = {1.0f, 1.0f, 1.0f};

	// -----------------------
	// 弾（複数弾）
	// -----------------------
	std::list<std::unique_ptr<PlayerBullet>> bullets_;

	// モデル
	Object3d* bulletModel_ = nullptr;

	// 弾の発射
	bool isShoot_ = false;

	// -----------------------
	// 照準
	// -----------------------

	std::unique_ptr<Primitive> reticle_;

	// 照準の3D座標
	Vector3 reticlePos3D_ = {0.0f, 0.0f, 50.0f};

	bool isReticleAutoFollow_ = true;

	// 【追加】レティクルの追従スピード（0.0f ～ 1.0f）
	float reticleFollowSpeed_ = 0.15f;

private:
	/// <summary>
	/// 移動処理
	/// </summary>
	void UpdateMove();

	/// <summary>
	/// ImGuiの描画
	/// </summary>
	void UpdateImGui();

	/// <summary>
	/// 攻撃処理
	/// </summary>
	void Attack();
};