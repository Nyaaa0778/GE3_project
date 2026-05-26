#pragma once
#include <Vector3.h>
#include "WorldTransform.h"

class Camera;

class RailCameraController {
public:
    void Initialize(Camera* camera, const Vector3& initialPosition);
    void Update();

    // ゲッター
    const WorldTransform& GetWorldTransform() const { return worldTransform_; }
    WorldTransform& GetWorldTransform() { return worldTransform_; }

    const Vector3& GetPosition() const { return worldTransform_.translation; }
    float GetSpeed() const { return speed_; }

private:
    Camera* camera_ = nullptr;
    WorldTransform worldTransform_; // レール（親オブジェクト）のTransform

    // レールに対するカメラの相対的な位置と角度
    Vector3 cameraOffset_ = {0.0f, 5.0f, -15.0f};
    Vector3 cameraRotation_ = {0.3f, 0.0f, 0.0f};

    // スクロール速度
    float speed_ = 0.5f;
};
