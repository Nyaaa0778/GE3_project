#include "Reticle.h"

#include <MyEngine.h>
#include <MathUtility.h>
#include "Camera.h"
#include "Sprite.h"
#include "Object3d.h"

using namespace MathUtility;

Reticle::Reticle() = default;
Reticle::~Reticle() = default;

void Reticle::Initialize(Camera* camera) {
	camera_ = camera;

	reticle_ = std::make_unique<Object3d>();
	reticle_->Initialize("sphere");
	reticle_->SetCamera(camera_);

	worldTransformReticle_.Initialize();
	worldTransformReticle_.scale = kReticleDrawSize;
	// 親子関係を設定せず、ワールド空間に直接配置する
	worldTransformReticle_.parent = nullptr;

	reticleSprite_ = std::make_unique<Sprite>();
	reticleSprite_->Initialize("reticle.png", {640.0f, 360.0f}, {0.5f, 0.5f});
	reticleSprite_->SetScale({kReticleDrawSize.x, kReticleDrawSize.y});
	reticleSprite_->SetColor({0.0f, 0.0f, 0.0f, 1.0f});
}

void Reticle::Update(const WorldTransform& playerWorldTransform) {
	const float kDistancePlayerToReticle = 50.0f;
	const float kReticleScaleX = 2.5f; // 照準の横方向の可動域倍率 (1.0f より大きい値で広がる)
	const float kReticleScaleY = 2.5f; // 照準の縦方向の可動域倍率 (1.0f より大きい値で広がる)

	Vector3 reticleWorldPos = {};

	if (playerWorldTransform.parent) {
		// 親（レールカメラ）のローカル空間で照準位置を計算（自機の位置に比例してさらに外側へ）
		Vector3 reticleLocalPos = {};
		reticleLocalPos.x = playerWorldTransform.translation.x * kReticleScaleX;
		reticleLocalPos.y = playerWorldTransform.translation.y * kReticleScaleY;
		reticleLocalPos.z = playerWorldTransform.translation.z + kDistancePlayerToReticle;

		// 親のワールド行列を使ってワールド空間の座標に変換する
		reticleWorldPos = MathUtility::Transform(reticleLocalPos, playerWorldTransform.parent->matWorld);
	} else {
		// 親がいない場合のフォールバック（従来通り正面方向へ配置）
		Vector3 forward = { playerWorldTransform.matWorld.m[2][0], playerWorldTransform.matWorld.m[2][1], playerWorldTransform.matWorld.m[2][2] };
		if (Length(forward) > 0.0001f) {
			forward = Normalize(forward);
		} else {
			forward = { 0.0f, 0.0f, 1.0f };
		}
		reticleWorldPos = playerWorldTransform.GetWorldPosition() + forward * kDistancePlayerToReticle;
	}

	// この座標を3Dレティクルのワールド座標（translation）として設定
	worldTransformReticle_.translation = reticleWorldPos;

	worldTransformReticle_.UpdateMatrix();

	reticle_->SetWorldTransform(&worldTransformReticle_);
	reticle_->Update();

	// 3Dレティクルのワールド座標から2Dレティクルのスクリーン座標を計算
	{
		Vector3 positionReticle = worldTransformReticle_.GetWorldPosition();
		// ビューポート行列
		Matrix4x4 matViewport = MakeViewportMatrix(0.0f, 0.0f, static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight), 0.0f, 1.0f);
		// ビュー行列、プロジェクション行列、ビューポート行列を合成する
		Matrix4x4 matViewProjectionViewport = camera_->matView * camera_->matProjection * matViewport;
		// ワールド→スクリーン座標変換
		positionReticle = MathUtility::Transform(positionReticle, matViewProjectionViewport);
		// スプライトのレティクルに座標設定
		reticleSprite_->SetPosition(Vector2(positionReticle.x, positionReticle.y));
	}
	reticleSprite_->Update();
}

void Reticle::Draw() {
	// 照準の描画
	reticle_->Draw();
	reticleSprite_->Draw();
}

Vector2 Reticle::Get2DPosition() const {
	if (reticleSprite_) {
		return reticleSprite_->GetPosition();
	}
	return { 0.0f, 0.0f };
}
