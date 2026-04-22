#include "DebugCamera.h"

#include "Input.h"

void DebugCamera::Initialize() {
	SetTranslate({0.0f, 0.0f, -50.0f});
	SetRotate({0.0f, 0.0f, 0.0f});

	// 行列の計算
	CalculateMatrix();
}

void DebugCamera::Update(Camera* target) {
    auto input = Input::GetInstance();

    bool isRightPressed = input->PushMouse(1);
    bool isMiddlePressed = input->PushMouse(2);
    Input::MouseMove mouseMove = input->GetMouseMove();

    float mouseDeltaX = static_cast<float>(mouseMove.lX);
    float mouseDeltaY = static_cast<float>(mouseMove.lY);

    Vector3 currentRotate = target->GetRotate();
    Vector3 currentTranslate = target->GetTranslate();

    // 回転（中クリック）
    if (isMiddlePressed) {
        currentRotate.y += mouseDeltaX * rotateSpeed_;
        currentRotate.x += mouseDeltaY * rotateSpeed_;
        target->SetRotate(currentRotate);
    }

    // 移動（右クリック）
    if (isRightPressed) {
        // カメラの現在のワールド行列を取得
        const Matrix4x4& matWorld = target->GetWorldMatrix();

        // 行列からカメラの「右方向」と「上方向」のベクトルを抽出する
        // matWorld.m[0][0]～[0][2] が X軸（右）
        // matWorld.m[1][0]～[1][2] が Y軸（上）
        Vector3 right = {matWorld.m[0][0], matWorld.m[0][1], matWorld.m[0][2]};
        Vector3 up = {matWorld.m[1][0], matWorld.m[1][1], matWorld.m[1][2]};

        // 移動量を計算
        // マウスを右に動かしたら、カメラの「右方向」へ
        // マウスを上に動かしたら、カメラの「上方向」へ
        float velX = -mouseDeltaX * moveSpeed_; // 逆向きがいいならマイナスを外す
        float velY = mouseDeltaY * moveSpeed_;

        currentTranslate.x += (right.x * velX) + (up.x * velY);
        currentTranslate.y += (right.y * velX) + (up.y * velY);
        currentTranslate.z += (right.z * velX) + (up.z * velY);

        target->SetTranslate(currentTranslate);
    }

    float wheel = static_cast<float>(mouseMove.lZ); // ホイールの回転量を取得
    if (wheel != 0) {
        // カメラの現在の行列から「正面方向(Forward)」を取り出す
        const Matrix4x4& matWorld = target->GetWorldMatrix();

        // 3列目が正面方向（Z軸）
        Vector3 forward = {matWorld.m[2][0], matWorld.m[2][1], matWorld.m[2][2]};

        // ホイールがプラスなら前、マイナスなら後ろへ
        float moveDistance = wheel * moveSpeed_ * 0.1f; // 感度は適宜調整してください

        currentTranslate.x += forward.x * moveDistance;
        currentTranslate.y += forward.y * moveDistance;
        currentTranslate.z += forward.z * moveDistance;

        target->SetTranslate(currentTranslate);
    }

    // 最後に計算
    target->CalculateMatrix();
}