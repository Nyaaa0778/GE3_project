#include "RailCameraController.h"

#include <MyEngine.h>
#include <MathUtility.h>
#include <cassert>

using namespace MathUtility;

RailCameraController::RailCameraController() = default;
RailCameraController::~RailCameraController() = default;

void RailCameraController::Initialize(Camera* camera) {
	assert(camera);
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation = camera_->GetTranslate();
	worldTransform_.rotation = camera_->GetRotate();
}

void RailCameraController::Update() {
	// ------------------------------------
	// 1. 移動と回転の処理（
	// ------------------------------------
	// Z軸方向に毎フレーム 0.1f ずつ移動させる（奥へ進む）
	worldTransform_.translation.z += 0.3f;

	// ------------------------------------
	// 2. 行列の計算
	// ------------------------------------
	// ワールドトランスフォームのワールド行列再計算
	worldTransform_.UpdateMatrix();

	// カメラオブジェクトにワールド行列・ビュー行列を設定し、WVPとGPUバッファを同期
	camera_->matWorld = worldTransform_.matWorld;
	camera_->matView = MakeInverseMatrix(worldTransform_.matWorld);
	camera_->UpdateViewProjection();


	// ------------------------------------
	// 3. デバッグ表示
	// ------------------------------------
#ifdef USE_IMGUI
	ImGui::Begin("Camera");
	// スライダーでカメラのtranslationを表示
	ImGui::DragFloat3("Translation", &worldTransform_.translation.x, 0.1f);
	// スライダーでカメラのrotationを表示
	ImGui::DragFloat3("Rotation", &worldTransform_.rotation.x, 0.1f);
	ImGui::End();
#endif
}