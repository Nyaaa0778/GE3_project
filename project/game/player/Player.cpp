#include "Player.h"

#include <MyEngine.h>
#include <MathUtility.h>
#include <TimeManager.h>

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

void Player::Update(const Vector3& railTranslation) {
    // ImGuiの描画
    UpdateImGui();

    // 移動処理
    UpdateMove(railTranslation);

    // -----------------------
    // 弾（複数弾）の更新
    // ----------------------

    UpdateBullets(railTranslation);

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

void Player::UpdateMove(const Vector3& railTranslation) {
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

    UpdateReticle(railTranslation);

    // モデルに座標を反映
    model_->SetPosition(pos_);
}

/// <summary>
/// 照準の更新
/// </summary>
void Player::UpdateReticle(const Vector3& railTranslation) {
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

    // レールの現在位置を足してワールド座標へ変換
    Vector3 worldReticlePos = reticlePos_ + railTranslation;

    // 計算した座標をReticleに渡す
    reticle_->SetPosition(worldReticlePos);
    reticle_->Update();
}

/// <summary>
/// 弾（複数弾）の更新
/// </summary>
void Player::UpdateBullets(const Vector3& railTranslation) {
    Input* input = Input::GetInstance();

    if (bulletCooldownTimer_ > 0.0f) {
        bulletCooldownTimer_ -= TimeManager::GetInstance()->GetDeltaTime();
    }

    // スペースキーで発射（0.0f 以下になったら撃てる）
    if (input->PushKey(DIK_SPACE)) {
        if(bulletCooldownTimer_ <= 0.0f)
        {
            // プレイヤーと照準のワールド座標を求める
            Vector3 worldPlayerPos = pos_ + railTranslation;
            Vector3 worldReticlePos = reticlePos_ + railTranslation;

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
