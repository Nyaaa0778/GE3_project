#include "Player.h"

#include <cassert>
#include <algorithm>
#include "Camera.h"

#include <MyEngine.h>
#include <MathUtility.h>

#include "Plane.h"
#include "Primitive.h"
#include "PlayerBullet.h"
#include "LockOn.h"
#include "EnemyBase.h"
#include "Logger.h"
#include "Random.h"

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

	reticleSprite_ = std::make_unique<Sprite>();
	reticleSprite_->Initialize("reticle.png", {640.0f, 360.0f}, {0.5f, 0.5f});
	reticleSprite_->SetScale({kReticleDrawSize.x, kReticleDrawSize.y});
	reticleSprite_->SetColor({0.0f, 0.0f, 0.0f, 1.0f});

	// コライダーの初期設定
	SetShape(ColliderShape::kSphere);
	SetSphere({ 0.5f });

	// 前フレームのワールド座標の初期化
	prevWorldPos_ = worldTransform_.GetWorldPosition();
}

void Player::Update() {
	// ------------------------------------
	// 本体
	// ------------------------------------
	
	// プレイヤーの回転を常に(0, 0, 0)にする（親であるレールカメラの向きに平行にする）
	worldTransform_.rotation = { 0.0f, 0.0f, 0.0f };

	// 移動処理
	UpdateMove();

	// トランスフォーム行列の更新と転送
	worldTransform_.UpdateMatrix();

	// モデルの更新
	model_->Update();

	// ------------------------------------
	// 照準
	// ------------------------------------

	UpdateReticle();


	// ------------------------------------
	// 弾
	// ------------------------------------
	
	// 攻撃
	Attack();
	
	// 弾の更新
	UpdateBullet();

	// 前フレームのワールド座標を保存
	prevWorldPos_ = worldTransform_.GetWorldPosition();
}

void Player::Draw() {
	// ------------------------------------
	// 本体
	// ------------------------------------
	model_->Draw();

	// ------------------------------------
	// 照準
	// ------------------------------------
	/*reticle_->Draw();
	reticleSprite_->Draw();*/

	// ------------------------------------
	// 弾
	// ------------------------------------
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
	const float kReticleScaleX = 2.5f; // 照準の横方向の可動域倍率 (1.0f より大きい値で広がる)
	const float kReticleScaleY = 2.5f; // 照準の縦方向の可動域倍率 (1.0f より大きい値で広がる)

	Vector3 reticleWorldPos = {};

	if (worldTransform_.parent) {
		// 親（レールカメラ）のローカル空間で照準位置を計算（自機の位置に比例してさらに外側へ）
		Vector3 reticleLocalPos = {};
		reticleLocalPos.x = worldTransform_.translation.x * kReticleScaleX;
		reticleLocalPos.y = worldTransform_.translation.y * kReticleScaleY;
		reticleLocalPos.z = worldTransform_.translation.z + kDistancePlayerToReticle;

		// 親のワールド行列を使ってワールド空間の座標に変換する
		reticleWorldPos = MathUtility::Transform(reticleLocalPos, worldTransform_.parent->matWorld);
	} else {
		// 親がいない場合のフォールバック（従来通り正面方向へ配置）
		Vector3 forward = { worldTransform_.matWorld.m[2][0], worldTransform_.matWorld.m[2][1], worldTransform_.matWorld.m[2][2] };
		if (Length(forward) > 0.0001f) {
			forward = Normalize(forward);
		} else {
			forward = { 0.0f, 0.0f, 1.0f };
		}
		reticleWorldPos = worldTransform_.GetWorldPosition() + forward * kDistancePlayerToReticle;
	}

	// この座標を3Dレティクルのワールド座標（translation）として設定
	worldTransformReticle_.translation = reticleWorldPos;

	worldTransformReticle_.UpdateMatrix();

	reticle_->SetWorldTransform(&worldTransformReticle_);
	reticle_->Update();

	// 3Dレティクルのワールド座標から2Dレティクルのスクリーン座標を計算
	{
		Vector3 positionReticle = worldTransformReticle_.GetWorldPosition();
		// ビューポート行列
		Matrix4x4 matViewport = MakeViewportMatrix(0.0f, 0.0f, static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight), 0.0f, 1.0f);
		// ビュー行列、プロジェクション行列、ビューポート行列を合成する
		Matrix4x4 matViewProjectionViewport = camera_->matView * camera_->matProjection * matViewport;
		// ワールド→スクリーン座標変換
		positionReticle = MathUtility::Transform(positionReticle, matViewProjectionViewport);
		// スプライトのレティクルに座標設定
		reticleSprite_->SetPosition(Vector2(positionReticle.x, positionReticle.y));
	}
	reticleSprite_->Update();
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

	if (input->TriggerKey(DIK_Q)) {
		isLockOnMode_ = !isLockOnMode_; // trueとfalseを反転させる
	}

	// クールダウンが終了しており、キーが押されていたら発射
	if (input->PushKey(DIK_SPACE)) {
		if(cooldownTimer_ <= 0.0f)
		{
			auto newBullet = std::make_unique<PlayerBullet>();

			// 自機のワールド座標を取得
			Vector3 spawnPos = worldTransform_.GetWorldPosition();

			// 3Dレティクルのワールド座標と自機のワールド座標から速度ベクトルを算出
			Vector3 shootDir = worldTransformReticle_.GetWorldPosition() - spawnPos;
			shootDir = Normalize(shootDir);

			if (isLockOnMode_ && lockOn_ && lockOn_->GetTarget()) {
				// ロックオン対象を取得
				EnemyBase* target = lockOn_->GetTarget();
				Vector3 targetPos = target->GetWorldPosition();

				// ターゲットへの方向ベクトル ＝ 終点(敵) － 始点(自機)
				shootDir = targetPos - spawnPos;
				shootDir = Normalize(shootDir);
			} else {
				// 通常モード（またはターゲットがいない場合）は従来通り3Dレティクルを狙う
				shootDir = worldTransformReticle_.GetWorldPosition() - spawnPos;
				shootDir = Normalize(shootDir);
			}

			// 自機の1フレームあたりの移動ベクトル（慣性）を計算
			Vector3 playerFrameVelocity = worldTransform_.GetWorldPosition() - prevWorldPos_;

			// 弾の速度（自機の速度＋射撃方向の弾速）
			bulletVelocity_ = playerFrameVelocity + shootDir * kBulletSpeed;

			// 弾を初期化（親は nullptr でワールド空間上に配置する）
			newBullet->Initialize(camera_, spawnPos, bulletVelocity_);

			// 弾を登録する
			bullets_.push_back(std::move(newBullet));

			// 自機のワールド移動速度（1フレーム移動量 * 60秒）を算出
			Vector3 playerVelocity = (worldTransform_.GetWorldPosition() - prevWorldPos_) * 60.0f;

			// 射撃方向の算出
			shootDir = Normalize(bulletVelocity_);
			// 球体自機の少し前方から射出する
			Vector3 effectSpawnPos = spawnPos + shootDir * 1.2f;

			// 1. 飛び散る火花 (方向・速度・色の異なるコーン状の広がり)
			for (uint32_t i = 0; i < 16; ++i) {
				// 弾道の周りに広がり（スプレッド）を加算
				// より直線的で鋭い火花にするため、範囲を -0.08f 〜 0.08f に絞る
				Vector3 randomSpread = Random::RangeVector3(-0.08f, 0.08f);
				Vector3 particleVelDir = Normalize(shootDir + randomSpread);

				// 速度をランダムにばらけさせる (秒速35.0〜75.0)
				float speed = Random::RangeFloat(35.0f, 75.0f);
				Vector3 sparkVel = playerVelocity + particleVelDir * speed;

				// 色のばらつき（白黄、オレンジ、赤オレンジ）を持たせて熱量を表現
				Vector4 sparkColor;
				float colorRand = Random::RangeFloat(0.0f, 1.0f);
				if (colorRand < 0.3f) {
					sparkColor = { 1.0f, 1.0f, 0.7f, 1.0f }; // 白黄 (最も熱いコア付近)
				} else if (colorRand < 0.7f) {
					sparkColor = { 1.0f, 0.6f, 0.1f, 1.0f }; // オレンジ (一般的な火花)
				} else {
					sparkColor = { 0.9f, 0.3f, 0.05f, 1.0f }; // 赤オレンジ (冷めかけた火花)
				}

				// サイズもランダムに揺らす (直径0.12f〜0.24f)
				float size = Random::RangeFloat(0.12f, 0.24f);
				Vector3 sparkScale = { size, size, size };

				// 寿命もランダム (0.10秒〜0.22秒)
				float sparkLifeTime = Random::RangeFloat(0.10f, 0.22f);

				ParticleManager::GetInstance()->Emit("CircleParticle", effectSpawnPos, sparkVel, sparkColor, sparkScale, sparkLifeTime, 1);
			}

			// 2. 銃口の閃光 (白熱コア)
			for (uint32_t i = 0; i < 4; ++i) {
				Vector3 randomSpread = Random::RangeVector3(-0.03f, 0.03f);
				Vector3 coreVelDir = Normalize(shootDir + randomSpread);
				float speed = Random::RangeFloat(5.0f, 12.0f);
				Vector3 coreVel = playerVelocity + coreVelDir * speed;

				Vector4 coreColor = { 1.0f, 1.0f, 0.8f, 1.0f }; // 高輝度の白黄
				float size = Random::RangeFloat(1.0f, 1.4f);
				Vector3 coreScale = { size, size, size };
				float coreLifeTime = Random::RangeFloat(0.05f, 0.09f); // 瞬時に消滅

				ParticleManager::GetInstance()->Emit("CircleParticle", effectSpawnPos, coreVel, coreColor, coreScale, coreLifeTime, 1);
			}

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

Vector2 Player::GetReticle2DPosition() const {
	if (reticleSprite_) {
		return reticleSprite_->GetPosition();
	}
	return { 0.0f, 0.0f };
}
