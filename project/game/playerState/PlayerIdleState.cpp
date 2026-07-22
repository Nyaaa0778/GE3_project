#include "PlayerIdleState.h"

#include <memory>

#include <Input.h>

#include "Player.h"
#include "PlayerMoveState.h"

void PlayerIdleState::Enter() {
}

void PlayerIdleState::Update() {
	auto input = Input::GetInstance();

	if (input->PushKey(DIK_W) || input->PushKey(DIK_S) || 
		input->PushKey(DIK_A) || input->PushKey(DIK_D))
	{
		player_->ChangeState(std::make_unique<PlayerMoveState>());
	}
}

void PlayerIdleState::Exit() {
}
