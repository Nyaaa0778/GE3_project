#include "Goal.h"
#include "Camera.h"
#include "TimeManager.h"
#include <cmath>

Goal::Goal() = default;
Goal::~Goal() = default;

void Goal::Initialize(const Vector3& pos, Camera* camera) {
	camera_ = camera;
	basePosition_ = pos;

	// トランスフォームの初期化
	worldTransform_.Initialize();
	worldTransform_.translation = pos;
	// ワープゾーンのシリンダーサイズに拡縮を調整 (X, Zは直径、Yは高さ)
	worldTransform_.scale = { 1.0f, 1.0f, 1.0f }; 

	// コライダーの初期設定（球形状、半径 0.5）
	SetShape(ColliderShape::kSphere);
	SetSphere({ 0.5f });

	// シリンダープリミティブを生成し初期化
	cylinder_ = std::make_unique<Cylinder>();
	cylinder_->SetCamera(camera_);
	cylinder_->Initialize("gradationLine.png");
	cylinder_->SetWorldTransform(&worldTransform_); // トランスフォーム共有
	cylinder_->SetBlendMode(PrimitiveRenderer::BlendMode::kAdd); // 加算合成で光らせる

	uvTranslation_ = { 0.0f, 0.0f };
}

void Goal::Update() {
	// 円柱の回転（くるくる回るワープゾーン演出）
	worldTransform_.rotation.y += 0.005f;

	// 行列更新
	worldTransform_.UpdateMatrix();

	// 3. UVスクロール（gradationLineが下から上に流れ続ける演出）
	uvTranslation_.y -= TimeManager::GetInstance()->GetDeltaTime() * 1.5f;

	colorTimer_ += TimeManager::GetInstance()->GetDeltaTime();

	// サイン波を使ってRGBの値を 0.0 ~ 1.0 の間でうねらせる（周期をずらして虹色に）
	color_.x = (std::sin(colorTimer_ * 3.0f) + 1.0f) * 0.5f;       // R
	color_.y = (std::sin(colorTimer_ * 5.0f + 2.0f) + 1.0f) * 0.5f; // G
	color_.z = (std::sin(colorTimer_ * 7.0f + 4.0f) + 1.0f) * 0.5f; // B
	color_.w = 1.0f; // アルファ値（透明度）

	// シリンダーの更新
	if (cylinder_) {
		cylinder_->SetColor(color_);
		cylinder_->SetUVTranslation(uvTranslation_);
		cylinder_->Update();
	}
}

void Goal::Draw() {
	if (cylinder_) {
		cylinder_->Draw();
	}
}

void Goal::OnCollision() {
}

Vector3 Goal::GetWorldPosition() {
	return worldTransform_.GetWorldPosition();
}

void Goal::DrawImGui(const char* windowName) {
#ifdef USE_IMGUI
	if (cylinder_) {
		cylinder_->DrawImGui(windowName);
	}
#endif
}
