#include "PlayerIdleState.h"

#include <memory>

#include <Input.h>

#include "Player.h"
#include "PlayerMoveState.h"
#include "PlayerDeathState.h"

void PlayerIdleState::Enter(Player* player) {
}

void PlayerIdleState::Update(Player* player) {
	auto input = Input::GetInstance();

	if(player->IsAlive())
	{
		if (input->PushKey(DIK_W) || input->PushKey(DIK_S) ||
			input->PushKey(DIK_A) || input->PushKey(DIK_D))
		{
			player->ChangeState(std::make_unique<PlayerMoveState>());
		}
	} else
	{
		player->ChangeState(std::make_unique<PlayerDeathState>());
	}

}

void PlayerIdleState::Exit(Player* player) {
}
