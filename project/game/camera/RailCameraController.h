#pragma once
#include <Vector3.h>
#include "WorldTransform.h"

class Camera;
class RailPath;

class RailCameraController {
public:
    void Initialize(Camera* camera, const Vector3& initialPosition);
    void Update();

    // --- ゲッター ---
    const WorldTransform& GetWorldTransform() const { return worldTransform_; }
    WorldTransform& GetWorldTransform() { return worldTransform_; }
    const Vector3& GetPosition()        const { return worldTransform_.translation; }

    // --- パス制御 ---
    void  SetRailPath(RailPath* path) { railPath_ = path; }
    void  SetScrollActive(bool active) { isScrollActive_ = active; }
    void  SetScrollSpeed(float speed) { splineSpeed_ = speed; }
    float GetProgress()              const { return t_; }
    void  SetProgress(float t) { t_ = t; }

private:
    // スクロール ON: パスに沿って移動
    void UpdateOnPath();
    // スクロール OFF: 直線前進（フォールバック）
    void UpdateLinear();
    // カメラ位置・回転を worldTransform_ に合わせて設定
    void ApplyCameraTransform();

    Camera* camera_ = nullptr;
    WorldTransform worldTransform_;

    // カメラのオフセット（レール座標系ローカル）
    Vector3 cameraOffset_ = {0.0f, 5.0f, -30.0f};
    Vector3 cameraRotation_ = {0.15f, 0.0f, 0.0f};

    // 直線前進速度
    float speed_ = 0.5f;

    // パス走行パラメータ
    RailPath* railPath_ = nullptr;
    float     t_ = 0.0f;     // 進捗 (0 〜 1)
    float     splineSpeed_ = 0.0012f;   // 1 フレームあたりの進捗量
    bool      isScrollActive_ = false;
};