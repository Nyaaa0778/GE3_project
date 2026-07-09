#include "IPlayerBullet.h"

#include <cassert>

#include <MyEngine.h>
#include <MathUtility.h>

void IPlayerBullet::Draw() {
	model_->Draw(&worldTransform_);
}

void IPlayerBullet::OnCollision() {
	// 弾が当たったら消滅フラグを立てる
	isDead_ = true;
}

Vector3 IPlayerBullet::GetWorldPosition() {
	return worldTransform_.GetWorldPosition();
}

Vector3 IPlayerBullet::GetPrevWorldPosition() {
	return prevWorldPos_;
}
