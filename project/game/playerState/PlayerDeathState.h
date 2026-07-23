#pragma once
#include "IPlayerState.h"
class PlayerDeathState : public IPlayerState {
public:
	// 状態開始
	void Enter(Player* player) override;

	// 更新
	void Update(Player* player) override;

	// 状態終了
	void Exit(Player* player) override;

private:
	// ディゾルブ用パラメータ
	float dissolveThreshold_ = 0.0f;
	float dissolveSpeed_ = 0.015f; // ディゾルブの進行速度
};