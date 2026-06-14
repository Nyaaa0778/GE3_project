#include "Player.h"

#include <cassert>
#include <algorithm>

#include <MyEngine.h>

void Player::Initialize(const Vector3& InitialPos, Object3d* model, Camera* camera) {
	// nullチェック
	assert(model);
	// モデルを借りてくる
	model_ = model;

	// nullチェック
	assert(camera);
	// カメラを借りてくる
	model_->SetCamera(camera);

	// トランスフォームの初期化
	worldTransform_.Initialize();
	worldTransform_.translation = InitialPos;

	// モデルに自身のトランスフォームをセット
	model_->SetWorldTransform(&worldTransform_);
}

void Player::Update() {

    // 移動処理
    UpdateMove();

	// トランスフォーム行列の更新と転送
	worldTransform_.UpdateMatrix();

	// モデルの更新
	model_->Update();
}

void Player::Draw() {
	model_->Draw();
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
    worldTransform_.translation.x += move.x * kBaseSpeed;
    worldTransform_.translation.y += move.y * kBaseSpeed;

    // 範囲を超えないように制限
    worldTransform_.translation.x = std::clamp(worldTransform_.translation.x, -kMoveLimitX, kMoveLimitX);
    worldTransform_.translation.y = std::clamp(worldTransform_.translation.y, -kMoveLimitY, kMoveLimitY);
}
