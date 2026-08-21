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
#include "IPlayerState.h"
#include "PlayerIdleState.h"
#include "PlayerMoveState.h"
#include "LockOn.h"
#include "EnemyBase.h"
#include "Logger.h"
#include "Random.h"
#include "TextureManager.h"

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

	// 初期ステートは待機状態
	ChangeState(std::make_unique<PlayerIdleState>());

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
	SetSphere({ 0.5f });

	// 前フレームのワールド座標の初期化
	prevWorldPos_ = worldTransform_.GetWorldPosition();

	// HPの初期化
	hp_ = 100.0f;

	// 生存フラグとディゾルブ設定の初期化
	isAlive_ = true;
	dissolveThreshold_ = 0.0f;
	useDissolve_ = false;

	model_->SetDissolveEnabled(false);
	model_->SetDissolveThreshold(0.0f);
	model_->SetDissolveEdgeWidth(0.04f);
	model_->SetDissolveEdgeColor(Vector4(1.0f, 0.4f, 0.0f, 1.0f)); // 炎のようなオレンジ

	// ディゾルブ用のマスクテクスチャをあらかじめ読み込んで設定
	TextureManager::GetInstance()->LoadTexture("resources/sprites/noise0.png");
	model_->SetDissolveNoiseTexture("resources/sprites/noise0.png");

	// プールの初期化
	normalBulletPool_.Initialize(kMaxBullets);
	homingBulletPool_.Initialize(kMaxBullets);
}

void Player::Update(const std::list<EnemyBase*>& enemies) {
	// ------------------------------------
	// 本体
	// ------------------------------------
	
	if (!isAlive_) {
		// ディゾルブを進行させる
		if (useDissolve_) {
			dissolveThreshold_ += (1.0f / kDissolveDuration) * TimeManager::GetInstance()->GetDeltaTime();
			if (dissolveThreshold_ > 1.0f) {
				dissolveThreshold_ = 1.0f;
			}
		}

		model_->SetDissolveEnabled(useDissolve_);
		model_->SetDissolveThreshold(dissolveThreshold_);

		// トランスフォーム行列の更新と転送
		worldTransform_.UpdateMatrix();

		// モデルの更新
		model_->Update();

		// ゲーム全体の動きを止めるため、既存の弾の更新も停止する
		// UpdateBullet(enemies);

		// 前フレームのワールド座標を保存
		prevWorldPos_ = worldTransform_.GetWorldPosition();
		return;
	}

	// プレイヤーの回転を常に(0, 0, 0)にする（親であるレールカメラの向きに平行にする）
	worldTransform_.rotation = { 0.0f, 0.0f, 0.0f };

	// ステートの更新
	if (currentState_)
	{
		currentState_->Update(this);
	}

	// トランスフォーム行列の更新と転送
	worldTransform_.UpdateMatrix();

	// モデルの更新
	model_->Update();

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
	// 完全にディゾルブしきるまでは描画する
	if (isAlive_ || dissolveThreshold_ < 1.0f) {
		model_->Draw();
	}

	// ------------------------------------
	// 照準
	// ------------------------------------
	// 生存時かつ、ロックオンモードではない場合のみ通常レティクルを描画
	if (isAlive_ && !isLockOnMode_) {
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
/// ステートチェンジ
/// </summary>
/// <param name="newState"></param>
void Player::ChangeState(std::unique_ptr<IPlayerState> newState) {
	// 古い状態があれば Exit を呼ぶ
	if (currentState_)
	{
		currentState_->Exit(this);
	}

	// 新しいステートに所有権を移動
	currentState_ = std::move(newState);

	// 新しいステートを開始
	currentState_->Enter(this);
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
	for (IPlayerBullet* bullet : bullets_) {
		bullet->Update(enemies);
	}

	bullets_.remove_if([this](IPlayerBullet* bullet) {
		if (bullet->IsDead()) {
			// 型を判定して適切なプールに返却
			if (auto* normal = dynamic_cast<NormalPlayerBullet*>(bullet)) {
				normalBulletPool_.Release(normal);
			} else if (auto* homing = dynamic_cast<HomingPlayerBullet*>(bullet)) {
				homingBulletPool_.Release(homing);
			}
			return true;
		}
		return false;
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
			bool canShoot = false;
			NormalPlayerBullet* newBullet = nullptr;
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

						// ホーミング弾をプールから取得・初期化
						HomingPlayerBullet* homingBullet = homingBulletPool_.Acquire();
						if (homingBullet) {
							PlayerBulletParam param;
							param.camera = camera_;
							param.position = spawnPos;
							param.velocity = targetBulletVelocity;
							param.target = target;
							homingBullet->Initialize(param);
							bullets_.push_back(homingBullet);
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

				// 通常の弾をプールから取得・初期化
				newBullet = normalBulletPool_.Acquire();
				if (newBullet) {
					PlayerBulletParam param;
					param.camera = camera_;
					param.position = spawnPos;
					param.velocity = bulletVelocity_;
					newBullet->Initialize(param);
					canShoot = true;
				}
			}

			if (canShoot && newBullet) {
				// 弾を登録する
				bullets_.push_back(newBullet);

				// クールダウンを設定
				cooldownTimer_ = kCooldownDuration;
			}
		}
	}
}

void Player::OnCollision() {
	if (!isAlive_) {
		return;
	}

	// 被弾時にHPを減らす
	hp_ -= 80.0f;
	if (hp_ <= 0.0f) {
		hp_ = 0.0f;
		isAlive_ = false;
		// ディゾルブ開始設定
		useDissolve_ = true;
		dissolveThreshold_ = 0.0f;
		model_->SetDissolveEnabled(useDissolve_);
		model_->SetDissolveThreshold(dissolveThreshold_);
	}
}

Vector3 Player::GetWorldPosition() {
	return worldTransform_.GetWorldPosition();
}


