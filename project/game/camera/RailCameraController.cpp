#include "RailCameraController.h"
#include "Camera.h"
#include "MathUtility.h"
#include "RailPath.h"

using namespace MathUtility;

// -------------------------------------------------------
//  初期化
// -------------------------------------------------------

void RailCameraController::Initialize(Camera* camera, const Vector3& initialPosition) {
    camera_ = camera;
    worldTransform_.Initialize();
    worldTransform_.translation = initialPosition;
    worldTransform_.rotation = {0.0f, 0.0f, 0.0f};
    t_ = 0.0f;
}

// -------------------------------------------------------
//  更新
// -------------------------------------------------------

void RailCameraController::Update() {
    if (isScrollActive_ && railPath_ && railPath_->GetPointCount() >= 2) {
        UpdateOnPath();
    } else {
        UpdateLinear();
    }
    ApplyCameraTransform();
}

// -------------------------------------------------------
//  private: パスに沿った移動
// -------------------------------------------------------

void RailCameraController::UpdateOnPath() {
    // 進捗を進める（0〜1 でループ）
    t_ += splineSpeed_;
    if (t_ > 1.0f) t_ -= 1.0f;

    worldTransform_.translation = railPath_->Evaluate(t_);

    // 進行方向を求めて yaw / pitch を計算
    float nextT = t_ + 0.001f;
    if (nextT > 1.0f) nextT -= 1.0f;

    const Vector3 dir = railPath_->Evaluate(nextT) - worldTransform_.translation;

    if (Length(dir) > 0.001f) {
        const Vector3 nd = Normalize(dir);
        const float   yaw = atan2f(nd.x, nd.z);
        const float   pitch = -atan2f(nd.y, sqrtf(nd.x * nd.x + nd.z * nd.z));
        worldTransform_.rotation = {pitch, yaw, 0.0f};
    }

    worldTransform_.UpdateMatrix();
}

// -------------------------------------------------------
//  private: 直線前進（フォールバック）
// -------------------------------------------------------

void RailCameraController::UpdateLinear() {
    worldTransform_.translation.z += speed_;
    worldTransform_.rotation = {0.0f, 0.0f, 0.0f};
    worldTransform_.UpdateMatrix();
}

// -------------------------------------------------------
//  private: カメラ位置・回転を適用
// -------------------------------------------------------

void RailCameraController::ApplyCameraTransform() {
    if (!camera_) return;

    // ローカルオフセットをレールの回転行列で変換してワールド座標に加算
    const Matrix4x4 rotMat = MakeRotateMatrix(worldTransform_.rotation);

    const Vector3 rotatedOffset = {
        cameraOffset_.x * rotMat.m[0][0] + cameraOffset_.y * rotMat.m[1][0] + cameraOffset_.z * rotMat.m[2][0],
        cameraOffset_.x * rotMat.m[0][1] + cameraOffset_.y * rotMat.m[1][1] + cameraOffset_.z * rotMat.m[2][1],
        cameraOffset_.x * rotMat.m[0][2] + cameraOffset_.y * rotMat.m[1][2] + cameraOffset_.z * rotMat.m[2][2],
    };

    camera_->SetTranslate(worldTransform_.translation + rotatedOffset);
    camera_->SetRotate(worldTransform_.rotation + cameraRotation_);
}