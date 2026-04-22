#pragma once

#include "Camera.h"

class DebugCamera : public Camera {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update(Camera* target);

private:
	// マウス操作による移動・回転の感度調整用
	float moveSpeed_ = 0.01f;
	float rotateSpeed_ = 0.001f;
};