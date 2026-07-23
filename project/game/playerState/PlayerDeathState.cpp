#include "PlayerDeathState.h"

#include "Player.h"

void PlayerDeathState::Enter(Player* player) {
	// 演出用変数の初期化
	dissolveThreshold_ = 0.0f;
}

void PlayerDeathState::Update(Player* player) {
	if (dissolveThreshold_ < 1.0f)
	{
		dissolveThreshold_ += dissolveSpeed_;

		if (dissolveThreshold_ > 1.0f)
		{
			dissolveThreshold_ = 1.0f;
		}

		// プレイヤーのモデル等にディゾルブ値を渡す
		// player->SetDissolveValue(dissolveThreshold_);

		// 演出が終わるまではここで Update を抜ける
		return;
	}
}

void PlayerDeathState::Exit(Player* player) {
}