#include "Reticle.h"

#include<MyEngine.h>

#include "Plane.h"

#include <cassert>

using namespace std;

Reticle::Reticle() = default;
Reticle::~Reticle() = default;

void Reticle::Initialize(Camera* camera, const Vector3& pos) {
	// nullチェック
	assert(camera);
	// カメラを保持
	camera_ = camera;

	// 板ポリの初期化
	plane_ = make_unique<Plane>();
	plane_->Initialize("circle.png");
	plane_->SetCamera(camera);
	plane_->SetScale(scale_);

	// 初期位置を設定
	pos_ = pos;
	plane_->SetPosition(pos_);
}

void Reticle::Update() {
	// 板ポリの更新
	plane_->Update();
}

void Reticle::Draw() {
	// 板ポリの描画
	plane_->Draw();
}

void Reticle::SetPosition(const Vector3& pos) {
	pos_ = pos;
	// 板ポリの実体に座標をセット
	plane_->SetPosition(pos_);
}