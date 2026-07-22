#include "PlayerMoveState.h"

#include <cmath>
#include <algorithm>

#include <Input.h>

#include "Player.h"
#include "PlayerIdleState.h"

void PlayerMoveState::Enter() {
}

void PlayerMoveState::Update() {
	// 移動方向ベクトル
	Vector3 move = {0.0f, 0.0f, 0.0f};

	// 入力取得
	Input* input = Input::GetInstance();

	// X軸（左右）
	if (input->PushKey(DIK_D)) { move.x += 1.0f; }
	if (input->PushKey(DIK_A)) { move.x -= 1.0f; }

	// Y軸（上下）
	if (input->PushKey(DIK_W)) { move.y += 1.0f; }
	if (input->PushKey(DIK_S)) { move.y -= 1.0f; }

	// 斜め移動の速度を一定にするための正規化
	float length = std::sqrt(move.x * move.x + move.y * move.y);
	if (length > 0.0f) {
		move.x /= length;
		move.y /= length;
	}

	// 速度を適用して移動
	player_->GetWorldTransform()->translation.x += move.x * kBaseSpeed;
	player_->GetWorldTransform()->translation.y += move.y * kBaseSpeed;

	// 範囲を超えないように制限
	player_->GetWorldTransform()->translation.x = std::clamp(player_->GetWorldTransform()->translation.x, -kMoveLimitX, kMoveLimitX);
	player_->GetWorldTransform()->translation.y = std::clamp(player_->GetWorldTransform()->translation.y, -kMoveLimitY, kMoveLimitY);
}

void PlayerMoveState::Exit() {
	player_->ChangeState(std::make_unique<PlayerIdleState>());
}
