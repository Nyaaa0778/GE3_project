#pragma once

class Player;

class IPlayerState {
public:

	// 仮想デストラクタ
	virtual ~IPlayerState() = default;

	// ------------------------------------
	// 純粋仮想関数
	// ------------------------------------

	// 状態開始
	virtual void Enter() = 0;

	// 更新
	virtual void Update() = 0;

	// 状態終了
	virtual void Exit() = 0;

	// ------------------------------------
	// Setter
	// ------------------------------------

	// プレイヤーをセット
	void SetPlayer(Player* player);

protected:
	Player* player_ = nullptr;
};

