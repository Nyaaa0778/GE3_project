#pragma once

#include "IScene.h"
#include "KeyframeAnimation.h"

#include <memory>

class Camera;
class DebugCamera;
class Object3d;

class GamePlayScene : public IScene {
public:
	GamePlayScene();
	~GamePlayScene();

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

private:
	std::unique_ptr<Camera> camera_;
	std::unique_ptr<DebugCamera> debugCamera_;
	bool useDebugCamera_ = false;

	std::unique_ptr<Object3d> animatedCube_;
	Animation cubeAnimation_;
	bool isAnimationPlaying_ = true;
	bool isAnimationLoop_ = true;
};


