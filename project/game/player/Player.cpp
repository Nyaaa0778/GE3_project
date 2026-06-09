#include "Player.h"

#include <MyEngine.h>
#include <MathUtility.h>
#include <TimeManager.h>
#include "WorldTransform.h"

using namespace MathUtility;

#include "Reticle.h" // 照準
#include "PlayerBullet.h" // 弾
#include "PlayerBulletPool.h"

#include <cassert> // nullチェック用
#include <cmath> // sqrt用
#include <algorithm> // min/max用

using namespace std;

Player::Player() = default;
Player::~Player() = default;

void Player::Initialize(Camera* camera, const Vector3& pos, Object3d* model, const std::string& bulletModelName) {
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
    // 弾（複数弾）
    // -----------------------
    bulletPool_ = std::make_unique<PlayerBulletPool>();
    bulletPool_->Initialize(camera, bulletModelName, 100); 

    // -----------------------
    // 照準
    // -----------------------

    // 実体生成
    reticle_ = make_unique<Reticle>();
    // 初期化
    reticle_->Initialize(camera, reticlePos_);
}

void Player::Update(const WorldTransform& railTransform) {
    // 移動処理
    UpdateMove(railTransform);

    // -----------------------
    // 弾（複数弾）の更新
    // ----------------------

    UpdateBullets(railTransform);

    // モデルの更新
    model_->Update();

    
}

void Player::Draw() {
    // -----------------------
    // 弾（複数弾）の描画
    // -----------------------

    for (const auto& bullet : bullets_) {
        bullet->Draw();
    }
    
    // -----------------------
    // 照準の描画
    // -----------------------

    reticle_->Draw();

    // モデルの描画
    model_->Draw();
}

void Player::UpdateMove(const WorldTransform& railTransform) {
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

    // 範囲を超えないように制限
    pos_.x = std::clamp(pos_.x, -kMoveLimit.x, kMoveLimit.x);
    pos_.y = std::clamp(pos_.y, -kMoveLimit.y, kMoveLimit.y);

    // -----------------------
    // 照準の更新
    // ----------------------

    UpdateReticle(railTransform);

    // モデルに座標を反映
    model_->SetPosition(pos_);
}

/// <summary>
/// 照準の更新
/// </summary>
void Player::UpdateReticle(const WorldTransform& railTransform) {
    // 1. 自機が移動制限枠のどの割合（-1.0 ～ 1.0）にいるかを計算
    float ratioX = pos_.x / kMoveLimit.x;
    float ratioY = pos_.y / kMoveLimit.y;

    // 2. レティクルの最大可動域（画面の端っこの座標）を設定
    const float kReticleLimitX = 14.0f;
    const float kReticleLimitY = 7.0f;

    // 3. 割合をレティクルの可動域に掛け合わせて、行きたい目標位置を出す
    Vector3 targetReticlePos;
    targetReticlePos.x = ratioX * kReticleLimitX;
    targetReticlePos.y = ratioY * kReticleLimitY;
    targetReticlePos.z = pos_.z + kDepthPos;

    // 4. 自機の動きにほんの少しだけ遅れてついてくるイージング
    float easing = 0.3f;
    reticlePos_.x += (targetReticlePos.x - reticlePos_.x) * easing;
    reticlePos_.y += (targetReticlePos.y - reticlePos_.y) * easing;
    reticlePos_.z = targetReticlePos.z;

    // レールの回転と平行移動を含むワールド行列を使ってワールド座標へ変換
    const Matrix4x4& m = railTransform.matWorld;
    Vector3 worldReticlePos = {
        reticlePos_.x * m.m[0][0] + reticlePos_.y * m.m[1][0] + reticlePos_.z * m.m[2][0] + m.m[3][0],
        reticlePos_.x * m.m[0][1] + reticlePos_.y * m.m[1][1] + reticlePos_.z * m.m[2][1] + m.m[3][1],
        reticlePos_.x * m.m[0][2] + reticlePos_.y * m.m[1][2] + reticlePos_.z * m.m[2][2] + m.m[3][2]
    };

    // 計算した座標をReticleに渡す
    reticle_->SetPosition(worldReticlePos);
    reticle_->Update();
}

/// <summary>
/// 弾（複数弾）の更新
/// </summary>
void Player::UpdateBullets(const WorldTransform& railTransform) {
    Input* input = Input::GetInstance();

    if (bulletCooldownTimer_ > 0.0f) {
        bulletCooldownTimer_ -= TimeManager::GetInstance()->GetDeltaTime();
    }

    // スペースキーで発射（0.0f 以下になったら撃てる）
    if (input->PushKey(DIK_SPACE)) {
        if(bulletCooldownTimer_ <= 0.0f)
        {
            // プレイヤーと照準のワールド座標を求める (レールのワールド行列を使用)
            const Matrix4x4& m = railTransform.matWorld;
            Vector3 worldPlayerPos = {
                pos_.x * m.m[0][0] + pos_.y * m.m[1][0] + pos_.z * m.m[2][0] + m.m[3][0],
                pos_.x * m.m[0][1] + pos_.y * m.m[1][1] + pos_.z * m.m[2][1] + m.m[3][1],
                pos_.x * m.m[0][2] + pos_.y * m.m[1][2] + pos_.z * m.m[2][2] + m.m[3][2]
            };
            
            Vector3 worldReticlePos = {
                reticlePos_.x * m.m[0][0] + reticlePos_.y * m.m[1][0] + reticlePos_.z * m.m[2][0] + m.m[3][0],
                reticlePos_.x * m.m[0][1] + reticlePos_.y * m.m[1][1] + reticlePos_.z * m.m[2][1] + m.m[3][1],
                reticlePos_.x * m.m[0][2] + reticlePos_.y * m.m[1][2] + reticlePos_.z * m.m[2][2] + m.m[3][2]
            };

            // 1. 照準への方向ベクトルを求める
            Vector3 direction = worldReticlePos - worldPlayerPos;

            // 2. ベクトルを正規化（長さを1にする）
            float length = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
            if (length > 0.0f) {
                direction.x /= length;
                direction.y /= length;
                direction.z /= length;
            }

            // 3. 正規化したベクトルに速度を掛ける
            const float kBulletSpeed = 0.5f; // 弾の速さ
            Vector3 velocity = {
                direction.x * kBulletSpeed,
                direction.y * kBulletSpeed,
                direction.z * kBulletSpeed
            };

            // 4. 弾を生成して初期化 (ワールド座標で発射)
            unique_ptr<PlayerBullet> newBullet = make_unique<PlayerBullet>();
            newBullet->Initialize(camera_, worldPlayerPos, velocity, bulletPool_.get());

            // 5. リストに登録
            bullets_.push_back(std::move(newBullet));

            bulletCooldownTimer_ = kBulletCooldown;
        }
    }

    // 全ての弾を更新
    for (const auto& bullet : bullets_) {
        bullet->Update();
    }

    // デスフラグが立っている弾をリストから一括削除
    bullets_.remove_if([](const unique_ptr<PlayerBullet>& bullet) {
        return bullet->IsDead();
                       });
}

/// <summary>
/// ImGuiの描画
/// </summary>
void Player::DrawImGuiInline() {
#ifdef USE_IMGUI
    // タブ内に直接描画するため、Begin/End や CollapsingHeader を省き、
    // すぐにコントロールを配置することで、スクロールを削減しスッキリした見た目にします。
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "👤 Player Transform");
    ImGui::Separator();
    
    // 位置(Position)の操作
    ImGui::Text("Position");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::DragFloat3("##PlayerPosition", &pos_.x, 0.1f, 0.0f, 0.0f, "X: %.1f  Y: %.1f  Z: %.1f")) {
        model_->SetPosition(pos_);
    }

    // 回転(Rotation)の操作
    ImGui::Text("Rotation");
    Vector3 rotate = model_->GetRotate();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::DragFloat3("##PlayerRotation", &rotate.x, 0.01f, -6.28f, 6.28f, "P: %.2f  Y: %.2f  R: %.2f")) {
        model_->SetRotation(rotate);
    }

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "🧪 Material Properties");
    ImGui::Separator();

    // 環境反射係数の操作
    float envCoeff = model_->GetEnvironmentCoefficient();
    ImGui::Text("Reflection Power");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##ReflectionPower", &envCoeff, 0.0f, 1.0f, "Power: %.2f")) {
        model_->SetEnvironmentCoefficient(envCoeff);
    }
#endif
}

Vector3 Player::GetWorldPos() const {
    if (model_) {
        const Matrix4x4& m = model_->GetWorldTransform().matWorld;
        return { m.m[3][0], m.m[3][1], m.m[3][2] };
    }
    return pos_;
}
