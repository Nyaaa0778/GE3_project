#pragma once

#include "IScene.h"

#include <memory>
#include <vector>
#include <string>
#include <json.hpp>

class Object3d;
class Camera;
class Player;
class Enemy;

class GamePlayScene : public IScene {
public:
	GamePlayScene();
	~GamePlayScene() override;

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

	// 配置再ロード
	void ReloadLevel();

	// ログ書き出し
	void SavePlayLog();

private:
	// カメラ
	std::unique_ptr<Camera> camera_;

	// 静的モデル
	std::vector<std::unique_ptr<Object3d>> objects_;

	// キャラクター
	std::unique_ptr<Player> player_;
	std::unique_ptr<Enemy> enemy_;

	// ログ管理
	std::vector<nlohmann::json> frameLogs_;
	int currentFrameIndex_ = 0;
	bool isRewindMode_ = false;

	// 変数パラメータ（ImGui調整可能）
	float playerSpeed_ = 0.1f;
	float enemySpeed_ = 0.05f;
	int enemyWaitTime_ = 60;
	int enemySeed_ = 42;
	bool triggerBug_ = false;
};
