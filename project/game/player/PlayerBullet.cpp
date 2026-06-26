#include "PlayerBullet.h"

#include <cassert>

#include <MyEngine.h>
#include <MathUtility.h>

using namespace MathUtility;

void PlayerBullet::Initialize(Camera* camera, const Vector3& pos, const Vector3& velocity) {
    model_ = std::make_unique<Object3d>();
    model_->Initialize("sphere");
    model_->SetCamera(camera);

    worldTransform_.Initialize();
    worldTransform_.translation = pos;
    worldTransform_.scale = {0.5f, 0.5f, 0.5f};
    //worldTransform_.camera_ = camera;
    velocity_ = velocity;

	// コライダーの初期設定 (球, 半径 0.2)
	SetShape(ColliderShape::kSphere);
	SetSphere({ 0.2f });

	prevWorldPos_ = pos;
}

void PlayerBullet::Update() {
	// 移動前のワールド座標を保存
	prevWorldPos_ = worldTransform_.GetWorldPosition();

	worldTransform_.translation += velocity_;
	worldTransform_.UpdateMatrix();
	model_->Update(&worldTransform_);

    if(!isDead_)
    {

        deathTimer_ -= TimeManager::GetInstance()->GetDeltaTime();

        if (deathTimer_ <= 0.0f) {
            isDead_ = true;
        }
    }
}

void PlayerBullet::Draw() {
	model_->Draw(&worldTransform_);
}

void PlayerBullet::OnCollision() {
	// 弾が当たったら消滅フラグを立てる
	isDead_ = true;
}

Vector3 PlayerBullet::GetWorldPosition() {
	return worldTransform_.GetWorldPosition();
}

Vector3 PlayerBullet::GetPrevWorldPosition() {
	return prevWorldPos_;
}
