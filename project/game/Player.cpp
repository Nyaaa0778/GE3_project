#include "Player.h"
#include "Input.h"
#include "MathUtility.h"

void Player::Initialize(Camera* camera, const Vector3& initialPosition) {
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize("cube"); // Player is cube
	obj_->SetCamera(camera);
	SetPosition(initialPosition);
	SetRotation({ 0.0f, 0.0f, 0.0f });
}

void Player::Update(bool isPaused) {
	if (!isPaused) {
		// WASD movement
		Input* input = Input::GetInstance();
		Vector3 move = { 0.0f, 0.0f, 0.0f };
		if (input->PushKey(DIK_W)) { move.z += 1.0f; }
		if (input->PushKey(DIK_S)) { move.z -= 1.0f; }
		if (input->PushKey(DIK_A)) { move.x -= 1.0f; }
		if (input->PushKey(DIK_D)) { move.x += 1.0f; }

		if (MathUtility::Length(move) > 0.0f) {
			move = MathUtility::Normalize(move);
			move = MathUtility::Multiply(move, speed_);
			position_ = MathUtility::Add(position_, move);
			obj_->SetPosition(position_);
		}
	}

	obj_->Update();
}

void Player::Draw() {
	if (obj_) {
		obj_->Draw();
	}
}

void Player::SetPosition(const Vector3& pos) {
	position_ = pos;
	if (obj_) {
		obj_->SetPosition(position_);
	}
}

void Player::SetRotation(const Vector3& rot) {
	rotation_ = rot;
	if (obj_) {
		obj_->SetRotation(rotation_);
	}
}
