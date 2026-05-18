#include "PlayerBullet.h"

#include <MyEngine.h>

#include <cassert>

void PlayerBullet::Initialize(Object3d* model, const Vector3& pos, const Vector3& direction) {
	// nullチェック
	assert(model);

	// モデルを借りる
	model_ = model;

	//初期位置を設定
	pos_ = pos;
	model_->SetPosition(pos_);

	// 方向ベクトル（長さ1）に速さを掛けて速度ベクトルにする
	velocity_.x = direction.x * kBaseSpeed;
	velocity_.y = direction.y * kBaseSpeed;
	velocity_.z = direction.z * kBaseSpeed;
}

void PlayerBullet::Update() {
	// 移動処理
	UpdateMove();

	// モデルの更新
	model_->Update();
}

void PlayerBullet::Draw() {
	// モデルの描画
	model_->Draw();
}

void PlayerBullet::UpdateMove() {
	// 速度分だけ位置を移動させる
	pos_.x += velocity_.x;
	pos_.y += velocity_.y;
	pos_.z += velocity_.z;

	// モデルに新しい座標をセット
	model_->SetPosition(pos_);
}
