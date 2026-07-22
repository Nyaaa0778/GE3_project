#pragma once

#include "IPlayerState.h"

#include <Vector3.h>

class PlayerMoveState : public IPlayerState {
public:
	// 状態開始
	void Enter() override;

	// 更新
	void Update() override;

	// 状態終了
	void Exit() override;

private:
	// 速さ
	float kBaseSpeed = 0.2f;

	// 移動制限
	static constexpr float kMoveLimitX = 7.0f;
	static constexpr float kMoveLimitY = 3.5f;
};

