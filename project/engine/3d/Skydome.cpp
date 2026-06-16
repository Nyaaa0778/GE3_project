#include "Skydome.h"

#include <cassert>

#include <MyEngine.h>

void Skydome::Initialize(Object3d* model) {
	// nullチェック
	assert(model);
	// モデルを借りる
	model_ = model;

	worldTransform_.Initialize();
}

void Skydome::Update() {
	// モデルの更新
	model_->Update();
}

void Skydome::Draw() {
	// モデルの描画
	model_->Draw();
}
