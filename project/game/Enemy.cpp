#include "Enemy.h"
#include "MathUtility.h"
#include <cmath>

void Enemy::Initialize(Camera* camera, const Vector3& initialPosition) {
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize("sphere"); // Enemy is sphere
	obj_->SetCamera(camera);
	SetPosition(initialPosition);
	SetRotation({ 0.0f, 0.0f, 0.0f });
	SetSeed(seed_);
	ChooseNewTarget();
}

void Enemy::SetSeed(int seed) {
	seed_ = seed;
	randGen_.seed(seed_);
}

void Enemy::ChooseNewTarget() {
	std::uniform_real_distribution<float> distPos(-10.0f, 10.0f);
	wanderTarget_ = { distPos(randGen_), 0.0f, distPos(randGen_) };
}

void Enemy::Update(bool isPaused) {
	if (!isPaused) {
		// Enemy AI logic
		if (state_ == State::Wander) {
			Vector3 dir = MathUtility::Subtract(wanderTarget_, position_);
			float dist = MathUtility::Length(dir);
			if (dist < 0.2f) {
				state_ = State::Wait;
				timer_ = waitTimeMax_;
			} else {
				dir = MathUtility::Normalize(dir);
				Vector3 move = MathUtility::Multiply(dir, speed_);
				position_ = MathUtility::Add(position_, move);
				obj_->SetPosition(position_);

				// 進行方向に向く
				float angle = std::atan2(dir.x, dir.z);
				rotation_.y = angle;
				obj_->SetRotation(rotation_);
			}
		} else if (state_ == State::Wait) {
			timer_--;
			if (timer_ <= 0) {
				state_ = State::Wander;
				ChooseNewTarget();
			}
		}

		// HPを減少させ、変化をログで確認できるようにする
		hp_ -= 0.05f;
		if (hp_ < 0.0f) {
			hp_ = 100.0f;
		}
	}

	obj_->Update();
}

void Enemy::Draw() {
	if (obj_) {
		obj_->Draw();
	}
}

void Enemy::SetPosition(const Vector3& pos) {
	position_ = pos;
	if (obj_) {
		obj_->SetPosition(position_);
	}
}

void Enemy::SetRotation(const Vector3& rot) {
	rotation_ = rot;
	if (obj_) {
		obj_->SetRotation(rotation_);
	}
}
