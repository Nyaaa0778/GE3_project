#pragma once

#include "IScene.h"

#include <memory>
#include <vector>

class Object3d;
class Camera;

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

	// カメラ
	std::unique_ptr<Camera> camera_;
};
