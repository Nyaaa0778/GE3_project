#include "Player.h"

#include <MyEngine.h>
#include <MathUtility.h>

using namespace MathUtility;

#include <cassert> // nullチェック用
#include <cmath> // sqrt用
#include <algorithm> // min/max用

/// <summary>
/// 初期化
/// </summary>
/// <param name="model">自機のモデル</param>
/// <param name="pos">初期位置</param>
void Player::Initialize(Camera* camera, Object3d* model, Object3d* bulletModel, const Vector3& pos) {
	// nullチェック
	assert(model);
	// モデルを借りる
	model_ = model;
    
    // モデルにカメラをセット
    model_->SetCamera(camera);

    // 初期位置を設定
    pos_ = pos;

    // カメラの保持
    camera_ = camera;

    // 照準スプライトの初期化
    reticle_ = std::make_unique<Plane>();
    reticle_->Initialize("circle.png");
    reticle_->SetCamera(camera);

    // -----------------------
    // 弾（複数弾）
    // -----------------------
    
    //nullチェック
    assert(bulletModel);
    // モデルを借りる
    bulletModel_ = bulletModel;

    // モデルにカメラをセット
    bulletModel_->SetCamera(camera);
}

/// <summary>
/// 更新
/// </summary>
void Player::Update() {
    // ImGuiの描画
    UpdateImGui();

	// 移動処理
	UpdateMove();

    Input* input = Input::GetInstance();
    if (input->TriggerKey(DIK_SPACE)) {
        Attack();
    }

    // 全ての弾を更新
    for (auto& bullet : bullets_) {
        bullet->Update();
    }

	// モデルの更新
	model_->Update();

    // -----------------------
    // 照準の更新
    // -----------------------

    // 自動追従が有効な場合のみ、自機の位置から目標座標を計算する
    if (isReticleAutoFollow_) {
        // 1. 本来レティクルが行くべき「目標座標」を計算
        Vector3 targetReticlePos;
        targetReticlePos.x = pos_.x;  // 自機のX座標に合わせる
        targetReticlePos.y = pos_.y;  // 自機のY座標に合わせる
        targetReticlePos.z = pos_.z + 50.0f; // 50m奥を狙う

        // 2. 線形補間（Lerp）で現在地を目標座標に滑らかに近づける
        reticlePos3D_.x += (targetReticlePos.x - reticlePos3D_.x) * reticleFollowSpeed_;
        reticlePos3D_.y += (targetReticlePos.y - reticlePos3D_.y) * reticleFollowSpeed_;
        reticlePos3D_.z += (targetReticlePos.z - reticlePos3D_.z) * reticleFollowSpeed_;
    }

    // 3. 計算（またはImGuiで操作）した座標を反映させる
    reticle_->SetPosition(reticlePos3D_);

    // 照準スプライト(Plane)の更新
    reticle_->Update();
}

/// <summary>
/// 描画
/// </summary>
void Player::Draw() {
    // 全ての弾を描画
    for (auto& bullet : bullets_) {
        bullet->Draw();
    }

    // モデルの描画
    model_->Draw();

    // 照準の描画
    reticle_->Draw();
}

/// <summary>
/// 移動処理
/// </summary>
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

/// <summary>
/// 攻撃処理
/// </summary>
void Player::Attack() {
    // 1. 新しい弾のインスタンスを作成
    auto newBullet = std::make_unique<PlayerBullet>();

    // 自機から照準に向かうベクトルを計算して正規化
    Vector3 direction = Normalize(reticlePos3D_ - pos_);

    // 2. 共有の弾モデル(bulletModel_)と、現在の自機位置、方向を渡して初期化
    newBullet->Initialize(bulletModel_, pos_, direction);

    // 3. リストに登録
    bullets_.push_back(std::move(newBullet));
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

        // ---------------------------------------------------
        // 【追加】レティクル関連のデバッグ設定
        // ---------------------------------------------------
        ImGui::Separator(); // 分かりやすく区切り線を入れる

        // 自動追従フラグのON/OFF切り替え
        ImGui::Checkbox("Reticle Auto Follow", &isReticleAutoFollow_);

        // 自動追従がONの時は値を弄らせない（読み取り専用）、OFFの時はドラッグで動かせるようにする
        if (isReticleAutoFollow_) {
            ImGui::Text("Reticle Pos: X:%.2f, Y:%.2f, Z:%.2f", reticlePos3D_.x, reticlePos3D_.y, reticlePos3D_.z);
        } else {
            ImGui::DragFloat3("Reticle Position", &reticlePos3D_.x, 0.1f);
        }

        // レティクルの追従速度調整スライダー
        ImGui::SliderFloat("Reticle Follow Speed", &reticleFollowSpeed_, 0.01f, 1.0f);
    }

    ImGui::End();
#endif
}