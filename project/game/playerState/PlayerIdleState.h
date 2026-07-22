#pragma once

#include "IPlayerState.h"

class PlayerIdleState : public IPlayerState {
public:
	// 状態開始
	void Enter() override;

	// 更新
	void Update() override;

	// 状態終了
	void Exit() override;
};

