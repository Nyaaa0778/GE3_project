#include "Player.h"

#include <MyEngine.h>
#include <MathUtility.h>

using namespace MathUtility;

#include "Reticle.h"

#include <cassert> // nullチェック用
#include <cmath> // sqrt用
#include <algorithm> // min/max用

using namespace std;

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

    // -----------------------
    // 照準
    // -----------------------

    // 実体生成
    reticle_ = make_unique<Reticle>();
    // 初期化
    reticle_->Initialize(camera, reticlePos_);
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
    
    reticle_->Draw();

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

    // -----------------------
    // 照準の追従処理を追加
    // -----------------------

    // 1. 自機が移動制限枠の「どの割合（-1.0 ～ 1.0）」にいるかを計算する
    float ratioX = pos_.x / kMoveLimitX;
    float ratioY = pos_.y / kMoveLimitY;

    // 2. レティクルの最大可動域（画面の端っこの座標）を設定
    // ※画面のサイズに合わせて、自機の制限枠より大きい数値を設定します
    const float kReticleLimitX = 14.0f;
    const float kReticleLimitY = 7.0f;

    // 3. 割合をレティクルの可動域に掛け合わせて、行きたい目標位置を出す
    Vector3 targetReticlePos;
    targetReticlePos.x = ratioX * kReticleLimitX;
    targetReticlePos.y = ratioY * kReticleLimitY;
    targetReticlePos.z = pos_.z + kDepthPos;

    // 4. 自機の動きに「ほんの少しだけ」遅れてついてくるイージング（ここはお好みで）
    // 1.0f にすると自機と完全に同期してピタッと動きます。
    // 0.2f～0.3f くらいにすると、操作に少しだけ「重厚感・手応え」が出ます。
    float easing = 0.3f;
    reticlePos_.x += (targetReticlePos.x - reticlePos_.x) * easing;
    reticlePos_.y += (targetReticlePos.y - reticlePos_.y) * easing;
    reticlePos_.z = targetReticlePos.z;

    // 計算した座標をReticleに渡す
    reticle_->SetPosition(reticlePos_);
    reticle_->Update();

    // モデルに座標を反映
    model_->SetPosition(pos_);
}

void Player::UpdateImGui() {
#ifdef USE_IMGUI
    // ★ウィンドウ名を共通の "Debug Window" に変更
    // ImGui::Begin は同じ名前で呼び出すと、自動的に同じウィンドウ内に中身が追加されます
    ImGui::Begin("Debug Window");

    // --- セクション: Player Settings ---
    // CollapsingHeader を使うことで、クリックで開閉できるようになります
    if (ImGui::CollapsingHeader("Player Settings", ImGuiTreeNodeFlags_DefaultOpen)) {

        // 位置(Position)の操作
        if (ImGui::DragFloat3("Player Position", &pos_.x, 0.1f)) {
            model_->SetPosition(pos_);
        }

        // 回転(Rotation)の操作
        Vector3 rotate = model_->GetRotate();
        if (ImGui::DragFloat3("Player Rotation", &rotate.x, 0.01f)) {
            model_->SetRotation(rotate);
        }

        // 環境反射係数の操作
        float envCoeff = model_->GetEnvironmentCoefficient();
        if (ImGui::SliderFloat("Reflection Power", &envCoeff, 0.0f, 1.0f)) {
            model_->SetEnvironmentCoefficient(envCoeff);
        }
    }

    ImGui::End();
#endif
}
