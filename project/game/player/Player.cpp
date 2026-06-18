#include "Player.h"

#include <cassert>
#include <algorithm>
#include "Camera.h"

#include <MyEngine.h>
#include <MathUtility.h>

#include "Plane.h"
#include "Primitive.h"
#include "PlayerBullet.h"
#include "Logger.h"

using namespace MathUtility;

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
	reticle_->Initialize("sphere");
	reticle_->SetCamera(camera_);

	worldTransformReticle_.Initialize();
	worldTransformReticle_.scale = kReticleDrawSize;
	// 親子関係を設定せず、ワールド空間に直接配置する
	worldTransformReticle_.parent = nullptr;

	// コライダーの初期設定
	SetShape(ColliderShape::kSphere);
	SetSphere({ 1.0f });
}

void Player::Update() {
	// 移動処理
	UpdateMove();

	// トランスフォーム行列の更新と転送
	worldTransform_.UpdateMatrix();

	// 照準の更新
	UpdateReticle();

	// 攻撃
	Attack();
	
	// 弾の更新
	UpdateBullet();

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

	// ① Z方向向きのオフセットベクトルを作り、自キャラと同じ回転をかける
	Vector3 offset = {0.0f, 0.0f, kDistancePlayerToReticle};
	Matrix4x4 rotateMatrix = MathUtility::MakeRotateMatrix(worldTransform_.rotation);
	offset = MathUtility::Transform(offset, rotateMatrix);

	// ② 自キャラ座標から、オフセットベクトル分進んだ座標が、3Dレティクルの座標となる
	Vector3 reticleWorldPos = worldTransform_.GetWorldPosition() + offset;

	// この座標を3Dレティクルのワールド座標（translation）として設定
	worldTransformReticle_.translation = reticleWorldPos;

	worldTransformReticle_.UpdateMatrix();

	reticle_->SetWorldTransform(&worldTransformReticle_);
	reticle_->Update();
}

/// <summary>
/// 弾の更新
/// </summary>
void Player::UpdateBullet() {
	// クールダウンタイマーの更新
	if (cooldownTimer_ > 0.0f) {
		cooldownTimer_ -= TimeManager::GetInstance()->GetDeltaTime();
	}

	// 弾の更新
	for (const auto& bullet : bullets_) {
		bullet->Update();
	}

	bullets_.remove_if([](const std::unique_ptr<PlayerBullet>& bullet) {
		// 条件に一致すれば true を返すだけで、自動的に delete される
		return bullet->IsDead();
	});
}

/// <summary>
/// 攻撃
/// </summary>
void Player::Attack() {
	auto* input = Input::GetInstance();

	// クールダウンが終了しており、キーが押されていたら発射
	if (input->PushKey(DIK_SPACE)) {
		if(cooldownTimer_ <= 0.0f)
		{
			auto newBullet = std::make_unique<PlayerBullet>();

			// 自機のワールド座標を取得
			Vector3 spawnPos = worldTransform_.GetWorldPosition();

			// 3Dレティクルのワールド座標と自機のワールド座標から速度ベクトルを算出
			bulletVelocity_ = worldTransformReticle_.GetWorldPosition() - spawnPos;
			bulletVelocity_ = Normalize(bulletVelocity_) * kBulletSpeed;

			// 弾を初期化（親は nullptr でワールド空間上に配置する）
			newBullet->Initialize(camera_, spawnPos, bulletVelocity_);

			// 弾を登録する
			bullets_.push_back(std::move(newBullet));

			// クールダウンを設定
			cooldownTimer_ = kCooldownDuration;
		}
	}
}

void Player::OnCollision() {
	// 被弾処理など（必要に応じて）
}

Vector3 Player::GetWorldPosition() {
	return worldTransform_.GetWorldPosition();
}
