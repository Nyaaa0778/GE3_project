#include "Player.h"

#include <MyEngine.h>
#include <MathUtility.h>

using namespace MathUtility;

#include <cassert> // nullチェック用
#include <cmath> // sqrt用
#include <algorithm> // min/max用

using namespace std;

#include "DebugManager.h"

Player::Player() = default;
Player::~Player() = default;

void Player::Initialize(Camera* camera, const Vector3& pos, Object3d* model) {
    // nullチェック
    assert(camera);
    // カメラの保持
    camera_ = camera;

    // nullチェック
    assert(model);
    // モデルを保持
    model_ = model;
    // モデルにカメラをセット
    model_->SetCamera(camera);

    // 初期位置を設定
    pos_ = pos;
}

void Player::Update() {
    // ImGuiの描画
    UpdateImGui();

    // 移動処理
    UpdateMove();

    // モデルの更新
    model_->Update();


}

void Player::Draw() {
    // -----------------------
    // 照準
    // -----------------------

    // モデルの描画
    model_->Draw();
}

void Player::UpdateMove() {
    // 移動方向ベクトル
    Vector3 move = {0.0f, 0.0f, 0.0f};

    // 入力取得
    Input* input = Input::GetInstance();

    // X軸（左右）
    if (input->PushKey(DIK_D)) { move.x += 1.0f; }
    if (input->PushKey(DIK_A)) { move.x -= 1.0f; }

    // Y軸（上下）
    if (input->PushKey(DIK_W)) { move.y += 1.0f; }
    if (input->PushKey(DIK_S)) { move.y -= 1.0f; }

    // 斜め移動の速度を一定にするための正規化
    float length = std::sqrt(move.x * move.x + move.y * move.y);
    if (length > 0.0f) {
        move.x /= length;
        move.y /= length;
    }

    // 速度を適用して移動
    pos_.x += move.x * kBaseSpeed;
    pos_.y += move.y * kBaseSpeed;

    // 移動制限
    const float kMoveLimitX = 7.0f;
    const float kMoveLimitY = 3.5f;

    // 範囲を超えないように制限
    pos_.x = std::clamp(pos_.x, -kMoveLimitX, kMoveLimitX);
    pos_.y = std::clamp(pos_.y, -kMoveLimitY, kMoveLimitY);

    // モデルに座標を反映
    model_->SetPosition(pos_);
}

void Player::UpdateImGui() {
#ifdef USE_IMGUI
    auto* dbg = DebugManager::GetInstance();
    dbg->WatchVec3("Player", "Position", &pos_);
#endif
}