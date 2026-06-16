#include "PlayerBullet.h"

#include <cassert>

#include <MyEngine.h>
#include <MathUtility.h>

using namespace MathUtility;

void PlayerBullet::Initialize(Camera* camera, const Vector3& pos, const Vector3& velocity, const WorldTransform* parent) {
    model_ = std::make_unique<Object3d>();
    model_->Initialize("cube");
    model_->SetCamera(camera);

    worldTransform_.Initialize();
    worldTransform_.translation = pos;
    worldTransform_.parent = parent;
    worldTransform_.camera_ = camera;
    velocity_ = velocity;
}

void PlayerBullet::Update() {
	worldTransform_.translation += velocity_;
	worldTransform_.UpdateMatrix();
	model_->Update(&worldTransform_);
}

void PlayerBullet::Draw() {
	model_->Draw(&worldTransform_);
}
