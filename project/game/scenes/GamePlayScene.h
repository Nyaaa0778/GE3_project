#pragma once

#include "IScene.h"

#include <memory>
#include <vector>

class Object3d;
class Camera;
class DebugCamera;
class RailCameraController;
class Player;
class Level;
class Skydome;

class GamePlayScene : public IScene {
public:
	GamePlayScene();
	~GamePlayScene();

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

private:
	// ------------------------------------
	// レベル
	// ------------------------------------

	std::unique_ptr<Level> level_;

	// ------------------------------------
	// 自機
	// ------------------------------------

	std::unique_ptr<Player> player_;
	// モデル
	std::unique_ptr<Object3d> playerModel_;

	// ------------------------------------
	// カメラ
	// ------------------------------------

	std::unique_ptr<Camera> camera_;

	// レールカメラ
	std::unique_ptr<RailCameraController> railCamera_;

	// デバッグカメラ
	std::unique_ptr<DebugCamera> debugCamera_;
	bool useDebugCamera_ = false;

	// ------------------------------------
	// 天球
	// ------------------------------------

	std::unique_ptr<Skydome> skydome_;

	// モデル
	std::unique_ptr<Object3d> skydomeModel_;

private:
	void UpdateImGui();
};