#include "Player.h"

#include <cassert>
#include <algorithm>

#include <MyEngine.h>
#include <MathUtility.h>

#include "Plane.h"
#include "Primitive.h"
#include "PlayerBullet.h"
#include "Logger.h"

using namespace MathUtility;

struct AABB {
	Vector3 min;
	Vector3 max;
};

Player::Player() = default;

Player::~Player() = default;

void Player::Initialize(const Vector3& InitialPos, Object3d* model, Camera* camera) {
	
	// ------------------------------------
	// 本体
	// ------------------------------------
	
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

	// ------------------------------------
	// カメラ
	// ------------------------------------

	// カメラを保持
	camera_ = camera;

	// ------------------------------------
	// 照準
	// ------------------------------------

	reticle_ = std::make_unique<Object3d>();
	reticle_->Initialize("cube");
	reticle_->SetCamera(camera_);
	//reticle_->SetScale({0.5f, 0.5f, 0.5f});

	worldTransformReticle_.Initialize();
	worldTransformReticle_.scale = kReticleDrawSize;
	worldTransformReticle_.parent = &worldTransform_;
}

void Player::Update() {

    // 移動処理
    UpdateMove();

	// トランスフォーム行列の更新と転送
	worldTransform_.UpdateMatrix();

	// 照準の更新
	UpdateReticle();

	Attack();
	// 弾の更新
	for (const auto& bullet : bullets_) {
		bullet->Update();
	}

	// モデルの更新
	model_->Update();
}

void Player::Draw() {
	model_->Draw();

	reticle_->Draw();

	// 弾の描画
	for (const auto& bullet : bullets_) {
		bullet->Draw();
	}
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

/// <summary>
/// 照準の描画
/// </summary>
void Player::UpdateReticle() {
	const float kDistancePlayerToReticle = 50.0f;

	// ローカル座標系で自機の前方に配置
	worldTransformReticle_.translation = {0.0f, 0.0f, kDistancePlayerToReticle};
	worldTransformReticle_.UpdateMatrix();

	reticle_->SetWorldTransform(&worldTransformReticle_);
	reticle_->Update();
}

void Player::Attack() {
	auto* input = Input::GetInstance();

	if (input->PushKey(DIK_SPACE)) {
		auto newBullet = std::make_unique<PlayerBullet>();

		bulletVelocity_ = worldTransformReticle_.GetWorldPosition() - worldTransform_.GetWorldPosition();
		bulletVelocity_ = Normalize(bulletVelocity_) * kBulletSpeed;

		// 初期化
		newBullet->Initialize(camera_, worldTransform_.GetWorldPosition(), bulletVelocity_);

		// 弾を登録する（所有権を bullet_ に渡す）
		bullets_.push_back(std::move(newBullet));
	}
}

