#pragma once

#include "IScene.h"

#include <memory>
#include <vector>

class Object3d;
class Camera;
class Player;

class GamePlayScene : public IScene {
public:
	GamePlayScene();
	~GamePlayScene();

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

private:
	// モデル
	std::unique_ptr<Object3d> obj_;
	std::vector<std::unique_ptr<Object3d>> objects_;

	std::unique_ptr<Player> player_;          // ← 追加: Player本体
	std::unique_ptr<Object3d> playerModel_;   // ← 変更: obj_ からリネーム (Playerに渡すモデル)

	// カメラ
	std::unique_ptr<Camera> camera_;
};
