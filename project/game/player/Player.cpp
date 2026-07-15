#include "Player.h"

#include <cassert>
#include <algorithm>
#include "Camera.h"

#include <MyEngine.h>
#include <MathUtility.h>

#include "Plane.h"
#include "Primitive.h"
#include "IPlayerBullet.h"
#include "NormalPlayerBullet.h"
#include "HomingPlayerBullet.h"
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

	reticle_ = std::make_unique<Reticle>();
	reticle_->Initialize(camera_);

	// コライダーの初期設定
	SetShape(ColliderShape::kSphere);
	SetSphere({0.5f});

	// 前フレームのワールド座標の初期化
	prevWorldPos_ = worldTransform_.GetWorldPosition();

	// HPの初期化
	hp_ = 100.0f;
}

void Player::Update(const std::list<EnemyBase*>& enemies) {
	// ------------------------------------
	// 本体
	// ------------------------------------

	// プレイヤーの回転を常に(0, 0, 0)にする（親であるレールカメラの向きに平行にする）
	worldTransform_.rotation = {0.0f, 0.0f, 0.0f};

	// 移動処理
	UpdateMove();

	// トランスフォーム行列の更新と転送
	worldTransform_.UpdateMatrix();

	// モデルの更新
	model_->Update();

	if (isAlive_) {
		if (hp_ <= 0.0f) {
			isAlive_ = false;
		}
	}

	// ------------------------------------
	// 照準
	// ------------------------------------

	reticle_->Update(worldTransform_);


	// ------------------------------------
	// 弾
	// ------------------------------------

	// 攻撃
	Attack();

	// 弾の更新
	UpdateBullet(enemies);

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
	// ロックオンモードではない場合のみ通常レティクルを描画
	if (!isLockOnMode_) {
		reticle_->Draw();
	}

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
/// 弾の更新
/// </summary>
void Player::UpdateBullet(const std::list<EnemyBase*>& enemies) {
	// クールダウンタイマーの更新
	if (cooldownTimer_ > 0.0f) {
		cooldownTimer_ -= TimeManager::GetInstance()->GetDeltaTime();
	}

	// 弾の更新
	for (const auto& bullet : bullets_) {
		bullet->Update(enemies);
	}

	bullets_.remove_if([](const std::unique_ptr<IPlayerBullet>& bullet) {
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
		if (cooldownTimer_ <= 0.0f) {
			bool canShoot = false;
			std::unique_ptr<IPlayerBullet> newBullet;
			Vector3 spawnPos = worldTransform_.GetWorldPosition();
			Vector3 shootDir = {};

			if (isLockOnMode_) {
				// ロックオンモード時は、ロックオン対象が存在する場合のみ射撃可能
				if (lockOn_ && !lockOn_->GetTargets().empty()) {
					const auto& targets = lockOn_->GetTargets();
					for (EnemyBase* target : targets) {
						Vector3 targetPos = target->GetWorldPosition();

						// ターゲットへの方向ベクトル ＝ 終点(敵) － 始点(自機)
						Vector3 targetShootDir = targetPos - spawnPos;
						targetShootDir = Normalize(targetShootDir);

						// 自機の1フレームあたりの移動ベクトル（慣性）を計算
						Vector3 playerFrameVelocity = worldTransform_.GetWorldPosition() - prevWorldPos_;
						// 弾の速度（自機の速度＋射撃方向の弾速）
						Vector3 targetBulletVelocity = playerFrameVelocity + targetShootDir * kBulletSpeed;

						// ホーミング弾を生成・初期化
						auto homingBullet = std::make_unique<HomingPlayerBullet>();
						PlayerBulletParam param;
						param.camera = camera_;
						param.position = spawnPos;
						param.velocity = targetBulletVelocity;
						param.target = target;
						homingBullet->Initialize(param);
						bullets_.push_back(std::move(homingBullet));

						// 自機のワールド移動速度（1フレーム移動量 * 60秒）を算出
						Vector3 playerVelocity = (worldTransform_.GetWorldPosition() - prevWorldPos_) * 60.0f;
						// 球体自機の少し前方から射出する
						Vector3 effectSpawnPos = spawnPos + targetShootDir * 1.2f;

						// 1. 飛び散る火花 (方向・速度・色の異なるコーン状の広がり)
						for (uint32_t i = 0; i < 16; ++i) {
							Vector3 randomSpread = Random::RangeVector3(-0.08f, 0.08f);
							Vector3 particleVelDir = Normalize(targetShootDir + randomSpread);

							float speed = Random::RangeFloat(35.0f, 75.0f);
							Vector3 sparkVel = playerVelocity + particleVelDir * speed;

							Vector4 sparkColor;
							float colorRand = Random::RangeFloat(0.0f, 1.0f);
							if (colorRand < 0.3f) {
								sparkColor = {1.0f, 1.0f, 0.7f, 1.0f}; // 白黄
							} else if (colorRand < 0.7f) {
								sparkColor = {1.0f, 0.6f, 0.1f, 1.0f}; // オレンジ
							} else {
								sparkColor = {0.9f, 0.3f, 0.05f, 1.0f}; // 赤オレンジ
							}

							float size = Random::RangeFloat(0.12f, 0.24f);
							Vector3 sparkScale = {size, size, size};
							float sparkLifeTime = Random::RangeFloat(0.10f, 0.22f);

							ParticleManager::GetInstance()->Emit("CircleParticle", effectSpawnPos, sparkVel, sparkColor, sparkScale, sparkLifeTime, 1);
						}

						// 2. 銃口の閃光 (白熱コア)
						for (uint32_t i = 0; i < 4; ++i) {
							Vector3 randomSpread = Random::RangeVector3(-0.03f, 0.03f);
							Vector3 coreVelDir = Normalize(targetShootDir + randomSpread);
							float speed = Random::RangeFloat(5.0f, 12.0f);
							Vector3 coreVel = playerVelocity + coreVelDir * speed;

							Vector4 coreColor = {1.0f, 1.0f, 0.8f, 1.0f};
							float size = Random::RangeFloat(1.0f, 1.4f);
							Vector3 coreScale = {size, size, size};
							float coreLifeTime = Random::RangeFloat(0.05f, 0.09f);

							ParticleManager::GetInstance()->Emit("CircleParticle", effectSpawnPos, coreVel, coreColor, coreScale, coreLifeTime, 1);
						}
					}

					// 射撃した後は、ロックオンをクリアする
					lockOn_->ClearTargets();

					// クールダウンを設定
					cooldownTimer_ = kCooldownDuration;
				}
			} else {
				// 通常モード時は常に射撃可能
				shootDir = reticle_->Get3DPosition() - spawnPos;
				shootDir = Normalize(shootDir);

				// 自機の1フレームあたりの移動ベクトル（慣性）を計算
				Vector3 playerFrameVelocity = worldTransform_.GetWorldPosition() - prevWorldPos_;
				// 弾の速度（自機の速度＋射撃方向の弾速）
				bulletVelocity_ = playerFrameVelocity + shootDir * kBulletSpeed;

				// 通常の弾を生成・初期化
				newBullet = std::make_unique<NormalPlayerBullet>();
				PlayerBulletParam param;
				param.camera = camera_;
				param.position = spawnPos;
				param.velocity = bulletVelocity_;
				newBullet->Initialize(param);
				canShoot = true;
			}

			if (canShoot) {
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
						sparkColor = {1.0f, 1.0f, 0.7f, 1.0f}; // 白黄 (最も熱いコア付近)
					} else if (colorRand < 0.7f) {
						sparkColor = {1.0f, 0.6f, 0.1f, 1.0f}; // オレンジ (一般的な火花)
					} else {
						sparkColor = {0.9f, 0.3f, 0.05f, 1.0f}; // 赤オレンジ (冷めかけた火花)
					}

					// サイズもランダムに揺らす (直径0.12f〜0.24f)
					float size = Random::RangeFloat(0.12f, 0.24f);
					Vector3 sparkScale = {size, size, size};

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

					Vector4 coreColor = {1.0f, 1.0f, 0.8f, 1.0f}; // 高輝度の白黄
					float size = Random::RangeFloat(1.0f, 1.4f);
					Vector3 coreScale = {size, size, size};
					float coreLifeTime = Random::RangeFloat(0.05f, 0.09f); // 瞬時に消滅

					ParticleManager::GetInstance()->Emit("CircleParticle", effectSpawnPos, coreVel, coreColor, coreScale, coreLifeTime, 1);
				}

				// クールダウンを設定
				cooldownTimer_ = kCooldownDuration;
			}
		}
	}
}

void Player::OnCollision() {
	// 被弾時にHPを減らす
	hp_ -= 0.5f;
	if (hp_ < 0.0f) {
		hp_ = 0.0f;
	}
}

Vector3 Player::GetWorldPosition() {
	return worldTransform_.GetWorldPosition();
}


