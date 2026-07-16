#pragma once

#include "WorldTransform.h"

class Object3d;

class Skydome {
public:
	void Initialize(Object3d* model);
	void Update();
	void Draw();

	const WorldTransform& GetWorldTransform() const { return worldTransform_; }
	WorldTransform& GetWorldTransform() { return worldTransform_; }

private:
	// ワールド変換データ
	WorldTransform worldTransform_;
	// モデル
	Object3d* model_ = nullptr;
};

