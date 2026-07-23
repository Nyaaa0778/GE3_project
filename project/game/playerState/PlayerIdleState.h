#pragma once

#include "IPlayerState.h"

class PlayerIdleState : public IPlayerState {
public:
	// 状態開始
	void Enter(Player* player) override;

	// 更新
	void Update(Player* player) override;

	// 状態終了
	void Exit(Player* player) override;
};

