#include "EnemyBase.h"
#include <Object3d.h>
#include <Camera.h>

EnemyBase::EnemyBase(const EnemyStatus& initialStatus) :
	status_(initialStatus), isAlive_(true) {
}

EnemyBase::~EnemyBase() = default;

void EnemyBase::Initialize(Camera* camera, const Vector3& pos, const std::string& modelName) {
	pos_ = pos;
	model_ = std::make_unique<Object3d>();
	model_->Initialize(modelName);
	model_->SetCamera(camera);
	model_->SetPosition(pos_);
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
