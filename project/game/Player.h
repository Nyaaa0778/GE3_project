#pragma once

#include <Vector3.h>

#include <memory>

class Camera;
class Object3d;

class Player {
public:
	Player();
	~Player();
public:
	void Initialize(Camera* camera, const Vector3& pos, Object3d* model);

	void Update();

	void Draw();

private:
	// カメラ
	Camera* camera_ = nullptr;

	// モデル
	Object3d* model_ = nullptr;

	// 位置
	Vector3 pos_ = {0.0f, 0.0f, 0.0f};
	// 速さ
	static inline const float kBaseSpeed = 0.1f;

	// 当たり判定の大きさ
	static inline const Vector3 collisionSize_ = {1.0f, 1.0f, 1.0f};

private:
	/// <summary>
	/// 移動処理
	/// </summary>
	void UpdateMove();

	/// <summary>
	/// ImGuiの描画
	/// </summary>
	void UpdateImGui();
};