#include "EnemyBase.h"
#include <Object3d.h>
#include <Camera.h>
#include <WorldTransform.h>

EnemyBase::EnemyBase(const EnemyStatus& initialStatus) :
	status_(initialStatus), isAlive_(true) {
}

EnemyBase::~EnemyBase() = default;

void EnemyBase::Initialize(Camera* camera, const Vector3& pos, const std::string& modelName, const WorldTransform* parentTransform) {
	pos_ = pos;
	model_ = std::make_unique<Object3d>();
	model_->Initialize(modelName);
	model_->SetCamera(camera);
	model_->SetPosition(pos_);
	if (parentTransform) {
		model_->GetWorldTransform().parent = parentTransform;
	}
}

void EnemyBase::Update(Player* player) {
	(void)player;
	if (!isAlive_) {
		return;
	}
	if (model_) {
		model_->SetPosition(pos_);
		model_->Update();
	}
}

void EnemyBase::Draw() {
	if (!isAlive_) {
		return;
	}
	if (model_) {
		model_->Draw();
	}
}

void EnemyBase::TakeDamage(int damage) {
	if (!isAlive_) {
		return;
	}

	// 現在の HP からダメージ分だけ減少させる
	status_.currentHp -= damage;

	if (status_.currentHp <= 0) {
		Die();
	}
}

void EnemyBase::Die() {
	isAlive_ = false;
}

Vector3 EnemyBase::GetWorldPosition() const {
	if (model_) {
		const Matrix4x4& m = model_->GetWorldTransform().matWorld;
		return { m.m[3][0], m.m[3][1], m.m[3][2] };
	}
	return pos_;
}
