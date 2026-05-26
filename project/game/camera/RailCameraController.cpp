#include "RailCameraController.h"
#include "Camera.h"
#include "MathUtility.h"

using namespace MathUtility;

void RailCameraController::Initialize(Camera* camera, const Vector3& initialPosition) {
    camera_ = camera;
    worldTransform_.Initialize();
    worldTransform_.translation = initialPosition;
}

void RailCameraController::Update() {
    // 1. レールを前進させる
    worldTransform_.translation.z += speed_;

    // 2. レールの行列を更新する（子オブジェクトが更新される前に必ず実行）
    worldTransform_.UpdateMatrix();

    // 3. カメラをレールの後ろに追従させる
    if (camera_) {
        Vector3 targetPos = worldTransform_.translation + cameraOffset_;
        camera_->SetTranslate(targetPos);
        camera_->SetRotate(cameraRotation_);
    }
}
