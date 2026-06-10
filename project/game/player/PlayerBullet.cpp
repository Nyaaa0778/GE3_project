#include "PlayerBullet.h"

#include <MyEngine.h>
#include <MathUtility.h>

using namespace MathUtility;

#include "PlayerBulletPool.h"

#include <cassert>

PlayerBullet::~PlayerBullet() {
	if (model_ && bulletPool_) {
		bulletPool_->Return(model_);
	}
}

void PlayerBullet::Initialize(Camera* camera, const Vector3& pos, const Vector3& velocity, PlayerBulletPool* bulletPool) {
	// nullチェック
	assert(camera);
	// カメラを保持
	camera_ = camera;

	// nullチェック
	assert(bulletPool);
	//プールを保持
	bulletPool_ = bulletPool;
	// モデルを借りる
	model_ = bulletPool_->Rent();
	// モデルのnullチェック
	assert(model_);

	// 大きさを調整
	model_->SetScale(kCollisionSize);

	// 初期位置を設定
	pos_ = pos;
	// 初期速度を設定
	velocity_ = velocity;
	deathTimer_ = kLifeTime; 
	isDead_ = false;
}

void PlayerBullet::Update() {
	UpdateMove();

	// 弾が既に消滅している場合は、モデルの更新処理を行わずに抜ける
	if (isDead_) {
		return;
	}

	// モデルの更新
	model_->Update();
}

void PlayerBullet::Draw() {
	// モデルの描画
	model_->Draw();
}

void PlayerBullet::UpdateMove() {
	// 時間経過でデスフラグを立てる
	if (--deathTimer_ <= 0) {
		isDead_ = true;
		return;
	}

	// 座標を移動させる
	pos_ += velocity_;

	// 位置をセット
	if (model_) {
		model_->SetPosition(pos_);
	}
}
