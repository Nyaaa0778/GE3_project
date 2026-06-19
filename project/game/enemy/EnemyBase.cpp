#include "EnemyBase.h"
#include "Object3d.h"
#include <cassert>

void EnemyBase::Initialize(Object3d* model, Camera* camera, const Vector3& pos) {
	assert(model);
	model_ = model;
	camera_ = camera;

	// トランスフォームの初期化
	worldTransform_.Initialize();
	worldTransform_.translation = pos;

	// モデルにカメラをセット（トランスフォームはUpdate/Draw時に引数で渡すため、ここではセットしない）
	model_->SetCamera(camera_);
}

void EnemyBase::Update() {
	worldTransform_.UpdateMatrix();
	model_->Update(&worldTransform_);
}

void EnemyBase::Draw() {
	model_->Draw(&worldTransform_);
}


void EnemyBase::OnCollision() {
	// 被弾時に生存フラグをfalseにする
	isAlive_ = false;
}

Vector3 EnemyBase::GetWorldPosition() {
	return worldTransform_.GetWorldPosition();
}
