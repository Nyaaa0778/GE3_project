#pragma once

#include <memory>

#include <Vector3.h>

#include "WorldTransform.h"

class Camera;

class RailCameraController {
public:
	RailCameraController();
	~RailCameraController();
	void Initialize(Camera* camera);

	void Update();

public:
	const WorldTransform* GetWorldTransform() const { return &worldTransform_; }
	WorldTransform* GetWorldTransform() { return &worldTransform_; }

private:
	// ワールド変換データ
	WorldTransform worldTransform_;

	// カメラ	
	Camera* camera_ = nullptr;
};

