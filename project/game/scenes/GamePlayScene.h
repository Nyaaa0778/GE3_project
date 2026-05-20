#pragma once

#include "IScene.h"

#include <Vector3.h>

#include <memory>

class Object3d;
class Camera;
class DebugCamera;
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
	// -----------------------
	// カメラ
	// -----------------------
	std::unique_ptr<Camera> camera_;

	// 初期位置
	static inline const Vector3 kInitialCameraPos = {0.0f, 0.0f, -20.0f};

	// デバッグカメラ
	std::unique_ptr<DebugCamera> debugCamera_;
	bool useDebugCamera_ = true;

	// -----------------------
	// 自機
	// -----------------------
	std::unique_ptr<Player> player_;

	// モデル
	std::unique_ptr<Object3d> playerModel_;
	// 自機の弾のモデル
	std::unique_ptr<Object3d> playerBulletModel_;

	// 初期位置
	static inline const Vector3 kInitialPlayerPos = {0.0f, 0.0f, 0.0f};

private:
	/// <summary>
	/// ImGuiの描画
	/// </summary>
	void UpdateImGui();
};
