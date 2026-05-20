#include "PlayerBullet.h"

#include <MyEngine.h>

#include <cassert>

void PlayerBullet::Initialize(Camera* camera, const Vector3& pos, Object3d* model) {
	// nullチェック
	assert(camera);
	// カメラを保持
	camera_ = camera;

	// nullチェック
	assert(model);
	// モデルを保持
	model_ = model;

	// 初期位置を設定
	pos_ = pos;
}

void PlayerBullet::Update() {
	// モデルの更新
	model_->Update();
}

void PlayerBullet::Draw() {
	// モデルの描画
	model_->Draw();
}
