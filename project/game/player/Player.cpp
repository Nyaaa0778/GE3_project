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
#include "WireframeObject.h"
#include "TimeManager.h"
#include <cmath>
#include <numbers>

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

	// 数式制御多面体ビジュアルの初期化
	InitializeGeometricVisual();
}

void Player::Update(const std::list<EnemyBase*>& enemies) {
	// ------------------------------------
	// 本体
	// ------------------------------------
	
	if (!isAlive_) {
		// プレイヤーの回転を親のレールカメラに平行に保つ（ブレ防止）
		worldTransform_.rotation = { 0.0f, 0.0f, 0.0f };

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

		// 多面体ビジュアルの更新（死亡ディゾルブ反映）
		float dt = TimeManager::GetInstance()->GetDeltaTime();
		UpdateGeometricVisual(dt);

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
	// 幾何学多面体ビジュアルの更新
	// ------------------------------------
	float dt = TimeManager::GetInstance()->GetDeltaTime();
	UpdateGeometricVisual(dt);

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

void Player::UpdateAnimationOnly() {
	// プレイヤーの回転を親のレールカメラに平行に保つ
	worldTransform_.rotation = { 0.0f, 0.0f, 0.0f };

	// トランスフォーム行列の更新と転送
	worldTransform_.UpdateMatrix();

	// モデルの更新
	if (model_) {
		model_->Update();
	}

	// 多面体ビジュアルの更新（回転アニメーション継続）
	float dt = TimeManager::GetInstance()->GetDeltaTime();
	UpdateGeometricVisual(dt);

	// 前フレームのワールド座標を保存
	prevWorldPos_ = worldTransform_.GetWorldPosition();
}

void Player::Draw() {
	// ------------------------------------
	// 本体（数式制御多面体ビジュアル）
	// ------------------------------------
	// 完全にディゾルブしきるまでは描画する（マテリアライズ未形成時は非表示）
	if ((isAlive_ || dissolveThreshold_ < 1.0f) && materializeProgress_ > 0.001f) {
		DrawGeometricVisual();
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
	hp_ -= 100.0f;
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

// ==============================================================================
// 数式制御多面体ビジュアル専用関数群
// ==============================================================================

/// <summary>
/// 幾何学多面体パーツの初期化
/// </summary>
void Player::InitializeGeometricVisual() {
	// ==========================================================================
	// コンパクトサイズ化（当たり判定半径0.5fに合わせた高密度クリスタル）
	// 背景の都市（シアン＆マゼンタ）と被らないよう、外殻を「ピュアホワイト」、
	// 中心核を「高輝度ゴールド」にし、内側からシアンとマゼンタが鮮烈に漏れる配色に再設計
	// ==========================================================================

	// 1. 外殻：正二十面体 (Icosahedron)
	// 半径 0.65f（背景の街からくっきりと浮き立つ純白の幾何ケージ）
	outerShell_ = std::make_unique<WireframeObject>();
	outerShell_->Initialize();
	outerShell_->CreateIcosahedron(0.65f);
	outerShell_->SetCamera(camera_);
	outerShell_->SetColor({1.0f, 1.0f, 1.0f, 1.0f}); // まばゆいピュアホワイト
	outerShell_->GetWorldTransform().parent = &worldTransform_;

	// 2. 中間層：正十二面体 (Dodecahedron)
	// 半径 0.42f（内側で回転するマゼンタの光フレーム）
	middleShell_ = std::make_unique<WireframeObject>();
	middleShell_->Initialize();
	middleShell_->CreateDodecahedron(0.42f);
	middleShell_->SetCamera(camera_);
	middleShell_->SetColor({1.0f, 0.05f, 0.80f, 0.85f}); // ビビッドマゼンタ
	middleShell_->GetWorldTransform().parent = &worldTransform_;

	// 3. ジャイロリング (XY & XZ)
	// 半径 0.52f
	gyroRingX_ = std::make_unique<WireframeObject>();
	gyroRingX_->Initialize();
	gyroRingX_->CreateRing(0.52f, 32, 0); // XY
	gyroRingX_->SetCamera(camera_);
	gyroRingX_->SetColor({0.0f, 1.0f, 1.0f, 0.70f}); // シアンリング
	gyroRingX_->GetWorldTransform().parent = &worldTransform_;

	gyroRingY_ = std::make_unique<WireframeObject>();
	gyroRingY_->Initialize();
	gyroRingY_->CreateRing(0.52f, 32, 2); // XZ
	gyroRingY_->SetCamera(camera_);
	gyroRingY_->SetColor({1.0f, 0.85f, 0.25f, 0.65f}); // アクセントゴールドリング
	gyroRingY_->GetWorldTransform().parent = &worldTransform_;

	// 4. 内側中心核 (Center Core: 当たり判定ビーコン)
	// 正八面体 半径 0.15f
	// 背景のシアン/マゼンタの街に絶対にない「エレクトリック・ゴールド」で自機中心を明示！
	centerCore_ = std::make_unique<WireframeObject>();
	centerCore_->Initialize();
	centerCore_->CreateOctahedron(0.15f);
	centerCore_->SetCamera(camera_);
	centerCore_->SetColor({1.0f, 0.92f, 0.20f, 1.0f}); // 超高輝度ゴールド（当たり判定中心）
	centerCore_->GetWorldTransform().parent = &worldTransform_;

	// 5. 内側偏心公転光核：シアン光 (Cyan Core)
	// 正八面体 半径 0.08f
	cyanCore_ = std::make_unique<WireframeObject>();
	cyanCore_->Initialize();
	cyanCore_->CreateOctahedron(0.08f);
	cyanCore_->SetCamera(camera_);
	cyanCore_->SetColor({0.0f, 1.0f, 1.0f, 1.0f}); // 高彩度シアン
	cyanCore_->GetWorldTransform().parent = &worldTransform_;

	// 6. 内側偏心公転光核：マゼンタ光 (Magenta Core)
	// 正八面体 半径 0.08f
	magentaCore_ = std::make_unique<WireframeObject>();
	magentaCore_->Initialize();
	magentaCore_->CreateOctahedron(0.08f);
	magentaCore_->SetCamera(camera_);
	magentaCore_->SetColor({1.0f, 0.0f, 0.85f, 1.0f}); // 高彩度マゼンタ
	magentaCore_->GetWorldTransform().parent = &worldTransform_;

	// 7. 前方幾何学ポインター (Forward Vector Pointer)
	// 進行方向（Z+）を指すゴールドクリスタル
	forwardPointer_ = std::make_unique<WireframeObject>();
	forwardPointer_->Initialize();
	forwardPointer_->CreateOctahedron(0.10f);
	forwardPointer_->SetCamera(camera_);
	forwardPointer_->SetColor({1.0f, 0.90f, 0.30f, 0.95f}); // ゴールドポインター
	forwardPointer_->GetWorldTransform().parent = &worldTransform_;
	forwardPointer_->GetWorldTransform().scale = {0.18f, 0.18f, 0.70f};
	forwardPointer_->GetWorldTransform().translation = {0.0f, 0.0f, 0.55f};

	visualTime_ = 0.0f;
	currentTilt_ = {0.0f, 0.0f, 0.0f};
	targetTilt_ = {0.0f, 0.0f, 0.0f};
}

/// <summary>
/// 幾何学多面体パーツのアニメーション・数式制御
/// </summary>
void Player::UpdateGeometricVisual(float deltaTime) {
	visualTime_ += deltaTime;

	// ----------------------------------------------------
	// 1. 移動入力に応じたリアクティブ・バンク / ティルト
	// ----------------------------------------------------
	auto* input = Input::GetInstance();
	float inputX = 0.0f;
	float inputY = 0.0f;
	if (input->PushKey(DIK_D)) inputX += 1.0f;
	if (input->PushKey(DIK_A)) inputX -= 1.0f;
	if (input->PushKey(DIK_W)) inputY += 1.0f;
	if (input->PushKey(DIK_S)) inputY -= 1.0f;

	// 目標ティルト角（左右バンクロールZ、上下ピッチX）
	targetTilt_.z = -inputX * 0.30f;
	targetTilt_.x = -inputY * 0.18f;

	// スムーズな補間 (Lerp)
	const float tiltLerpSpeed = 10.0f;
	currentTilt_.x += (targetTilt_.x - currentTilt_.x) * (std::min)(1.0f, tiltLerpSpeed * deltaTime);
	currentTilt_.y += (targetTilt_.y - currentTilt_.y) * (std::min)(1.0f, tiltLerpSpeed * deltaTime);
	currentTilt_.z += (targetTilt_.z - currentTilt_.z) * (std::min)(1.0f, tiltLerpSpeed * deltaTime);

	// ----------------------------------------------------
	// 2. 数式制御の非同期回転（マテリアライズ高速スピンアップ対応）
	// ----------------------------------------------------
	float spinBoost = 1.0f;
	if (materializeProgress_ < 1.0f && materializeProgress_ > 0.0f) {
		spinBoost = 1.0f + (1.0f - materializeProgress_) * 7.5f; // 最大8.5倍の超高速スピン
	}

	// 外殻二十面体：ゆったりとした2軸回転
	outerRotation_.y += 0.35f * spinBoost * deltaTime;
	outerRotation_.x += 0.22f * spinBoost * deltaTime;

	// 中間十二面体：外殻とは逆回転＋別軸の角速度
	middleRotation_.y -= 0.50f * spinBoost * deltaTime;
	middleRotation_.z += 0.32f * spinBoost * deltaTime;
	middleRotation_.x -= 0.15f * spinBoost * deltaTime;

	// ジャイロリング：軌道回転
	gyroRotationX_.z += 0.75f * spinBoost * deltaTime;
	gyroRotationX_.x += 0.30f * spinBoost * deltaTime;
	gyroRotationY_.y -= 0.65f * spinBoost * deltaTime;
	gyroRotationY_.z -= 0.25f * spinBoost * deltaTime;

	// 中心核：微細なスピン
	centerCoreRotation_.y += 0.90f * spinBoost * deltaTime;
	centerCoreRotation_.x += 0.45f * spinBoost * deltaTime;

	// ----------------------------------------------------
	// 3. 幾何学的呼吸パルス（Mathematical Breathing Pulse）
	// ----------------------------------------------------
	float pulseOuter = 1.0f + 0.035f * std::sin(visualTime_ * 1.8f);
	float pulseMiddle = 1.0f + 0.050f * std::sin(visualTime_ * 2.5f + 1.57f);
	float pulseCore = 1.0f + 0.080f * std::sin(visualTime_ * 4.0f);

	// ----------------------------------------------------
	// 4. 内側光核の公転（Cyan & Magenta Twin Cores）
	// ----------------------------------------------------
	// 死亡ディゾルブ進行によるパーツごとの段階的残存度 (1.0 -> 0.0)
	float outerHealth = 1.0f;
	float midHealth   = 1.0f;
	float coreHealth  = 1.0f;
	if (useDissolve_ || dissolveThreshold_ > 0.0f) {
		// 外殻：0.0f 〜 0.35f で崩壊・剥落
		outerHealth = (std::clamp)(1.0f - (dissolveThreshold_ - 0.00f) / 0.35f, 0.0f, 1.0f);
		// 中間層：0.30f 〜 0.68f で軸ブレ・爆散
		midHealth   = (std::clamp)(1.0f - (dissolveThreshold_ - 0.30f) / 0.38f, 0.0f, 1.0f);
		// 中心核：0.65f 〜 1.00f で公転拡散・臨界崩壊
		coreHealth  = (std::clamp)(1.0f - (dissolveThreshold_ - 0.65f) / 0.35f, 0.0f, 1.0f);
	}

	// コア崩壊時は公転半径が遠心力で外側へ螺旋拡散
	float orbitRadius = 0.20f;
	float orbitSpeed = 2.4f;
	if (midHealth < 1.0f) {
		orbitSpeed = 6.0f; // 中間層が壊れると高速化
	}
	if (coreHealth < 1.0f) {
		orbitRadius += (1.0f - coreHealth) * 1.6f; // 遠心力で外側へ大きく螺旋拡散
		orbitSpeed = 14.0f + (1.0f - coreHealth) * 12.0f; // 猛スピードで螺旋拡散
	}
	float orbitAngle = visualTime_ * orbitSpeed;

	Vector3 cyanPos = {
		std::cos(orbitAngle) * orbitRadius,
		std::sin(orbitAngle * 2.0f) * (0.08f + (1.0f - coreHealth) * 0.5f),
		std::sin(orbitAngle) * orbitRadius
	};
	Vector3 magentaPos = {
		-cyanPos.x,
		-cyanPos.y,
		-cyanPos.z
	};

	// ----------------------------------------------------
	// 5. 各パーツのトランスフォーム設定・更新 (ライン単位の漸進的形成・崩壊 drawRatio_ 対応)
	// ----------------------------------------------------
	float coreProg = (std::clamp)((materializeProgress_ - 0.0f) / 0.40f, 0.0f, 1.0f);
	float midProg  = (std::clamp)((materializeProgress_ - 0.20f) / 0.50f, 0.0f, 1.0f);
	float outerProg= (std::clamp)((materializeProgress_ - 0.45f) / 0.55f, 0.0f, 1.0f);

	auto MaterializeScale = [](float prog) {
		if (prog <= 0.0f) return 0.0f;
		if (prog >= 1.0f) return 1.0f;
		return 0.82f + 0.18f * (prog * prog * (3.0f - 2.0f * prog));
	};
	float scaleCoreMult = (coreProg >= 1.0f) ? 1.0f : MaterializeScale(coreProg);
	float scaleMidMult  = (midProg >= 1.0f)  ? 1.0f : MaterializeScale(midProg);
	float scaleOuterMult= (outerProg >= 1.0f)? 1.0f : MaterializeScale(outerProg);

	// 外殻二十面体（0.0〜0.35で線が欠落しながら外側へ急拡大・弾け飛び）
	if (outerShell_) {
		auto& tf = outerShell_->GetWorldTransform();
		// 崩壊が進むにつれて回転が急加速
		float spinAccel = (outerHealth < 1.0f) ? (1.0f - outerHealth) * 5.0f : 0.0f;
		tf.rotation = {
			outerRotation_.x + currentTilt_.x + spinAccel * 0.7f,
			outerRotation_.y + currentTilt_.y + spinAccel,
			outerRotation_.z + currentTilt_.z + spinAccel * 0.5f
		};
		// 外側へ破断拡大 (1.0 -> 2.4倍)
		float breakScale = (outerHealth < 1.0f) ? (1.0f + std::pow(1.0f - outerHealth, 1.2f) * 1.5f) : 1.0f;
		float s = pulseOuter * scaleOuterMult * breakScale;
		tf.scale = {s, s, s};
		// 破片が前方へ少し吹き飛ぶ
		tf.translation = { 0.0f, 0.0f, (outerHealth < 1.0f) ? (1.0f - outerHealth) * 0.5f : 0.0f };
		outerShell_->SetDrawRatio(outerProg * outerHealth);
		outerShell_->Update();
	}

	// 中間十二面体（0.30〜0.68で線が欠落しながら爆散）
	if (middleShell_) {
		auto& tf = middleShell_->GetWorldTransform();
		float spinAccel = (midHealth < 1.0f) ? (1.0f - midHealth) * 6.5f : 0.0f;
		tf.rotation = {
			middleRotation_.x + currentTilt_.x - spinAccel * 0.4f,
			middleRotation_.y + currentTilt_.y - spinAccel,
			middleRotation_.z + currentTilt_.z + spinAccel * 0.8f
		};
		float breakScale = (midHealth < 1.0f) ? (1.0f + std::pow(1.0f - midHealth, 1.2f) * 1.8f) : 1.0f;
		float s = pulseMiddle * scaleMidMult * breakScale;
		tf.scale = {s, s, s};
		middleShell_->SetDrawRatio(midProg * midHealth);
		middleShell_->Update();
	}

	// ジャイロリング（過負荷で不均等に歪みながら爆散）
	if (gyroRingX_) {
		auto& tf = gyroRingX_->GetWorldTransform();
		float spinAccel = (midHealth < 1.0f) ? (1.0f - midHealth) * 8.0f : 0.0f;
		tf.rotation = {
			gyroRotationX_.x + currentTilt_.x + spinAccel * 0.5f,
			gyroRotationX_.y + currentTilt_.y,
			gyroRotationX_.z + currentTilt_.z + spinAccel
		};
		float breakX = (midHealth < 1.0f) ? (1.0f + (1.0f - midHealth) * 2.2f) : 1.0f;
		float breakY = (midHealth < 1.0f) ? (1.0f + (1.0f - midHealth) * 1.2f) : 1.0f;
		tf.scale = {scaleMidMult * breakX, scaleMidMult * breakY, scaleMidMult * breakX};
		gyroRingX_->SetDrawRatio(midProg * midHealth);
		gyroRingX_->Update();
	}

	if (gyroRingY_) {
		auto& tf = gyroRingY_->GetWorldTransform();
		float spinAccel = (midHealth < 1.0f) ? (1.0f - midHealth) * 8.0f : 0.0f;
		tf.rotation = {
			gyroRotationY_.x + currentTilt_.x,
			gyroRotationY_.y + currentTilt_.y - spinAccel,
			gyroRotationY_.z + currentTilt_.z - spinAccel * 0.5f
		};
		float breakX = (midHealth < 1.0f) ? (1.0f + (1.0f - midHealth) * 1.3f) : 1.0f;
		float breakZ = (midHealth < 1.0f) ? (1.0f + (1.0f - midHealth) * 2.4f) : 1.0f;
		tf.scale = {scaleMidMult * breakX, scaleMidMult * breakX, scaleMidMult * breakZ};
		gyroRingY_->SetDrawRatio(midProg * midHealth);
		gyroRingY_->Update();
	}

	// 中心核（臨界膨張・超爆散）
	if (centerCore_) {
		auto& tf = centerCore_->GetWorldTransform();
		float spinAccel = (coreHealth < 1.0f) ? (1.0f - coreHealth) * 12.0f : 0.0f;
		tf.rotation = {
			centerCoreRotation_.x + currentTilt_.x + spinAccel * 0.6f,
			centerCoreRotation_.y + currentTilt_.y + spinAccel,
			centerCoreRotation_.z + currentTilt_.z
		};
		// 臨界爆散直前に一気に巨大化して弾け飛ぶ (1.0 -> 3.2倍)
		float explodeScale = (coreHealth < 1.0f)
			? (1.0f + std::pow(1.0f - coreHealth, 1.5f) * 2.4f)
			: 1.0f;
		float s = pulseCore * scaleCoreMult * explodeScale;
		tf.scale = {s, s, s};
		centerCore_->SetDrawRatio(coreProg * coreHealth);
		centerCore_->Update();
	}

	// シアン光核
	if (cyanCore_) {
		auto& tf = cyanCore_->GetWorldTransform();
		tf.translation = cyanPos * scaleCoreMult;
		tf.rotation = {centerCoreRotation_.y * 2.5f, centerCoreRotation_.x * 2.5f, 0.0f};
		float coreScale = (coreHealth < 1.0f) ? (coreHealth * (1.0f + (1.0f - coreHealth) * 1.5f)) : 1.0f;
		tf.scale = {scaleCoreMult * coreScale, scaleCoreMult * coreScale, scaleCoreMult * coreScale};
		cyanCore_->SetDrawRatio(coreProg * coreHealth);
		cyanCore_->Update();
	}

	// マゼンタ光核
	if (magentaCore_) {
		auto& tf = magentaCore_->GetWorldTransform();
		tf.translation = magentaPos * scaleCoreMult;
		tf.rotation = {-centerCoreRotation_.y * 2.5f, -centerCoreRotation_.x * 2.5f, 0.0f};
		float coreScale = (coreHealth < 1.0f) ? (coreHealth * (1.0f + (1.0f - coreHealth) * 1.5f)) : 1.0f;
		tf.scale = {scaleCoreMult * coreScale, scaleCoreMult * coreScale, scaleCoreMult * coreScale};
		magentaCore_->SetDrawRatio(coreProg * coreHealth);
		magentaCore_->Update();
	}

	// 前方ポインター（外殻と同時に破損）
	if (forwardPointer_) {
		auto& tf = forwardPointer_->GetWorldTransform();
		tf.rotation = currentTilt_;
		float breakZ = (outerHealth < 1.0f) ? (1.0f + (1.0f - outerHealth) * 1.5f) : 1.0f;
		tf.scale = {scaleOuterMult, scaleOuterMult, scaleOuterMult * breakZ};
		tf.translation = {0.0f, 0.0f, 0.55f + (1.0f - outerHealth) * 0.8f};
		forwardPointer_->SetDrawRatio(outerProg * outerHealth);
		forwardPointer_->Update();
	}

	// ----------------------------------------------------
	// 6. カラー・ショート明滅制御
	// ----------------------------------------------------
	// 形成中の明滅
	float matGlow = 1.0f;
	if (materializeProgress_ < 1.0f && materializeProgress_ > 0.0f) {
		matGlow = 1.0f + 0.5f * std::sin(visualTime_ * 32.0f);
	}

	// 外殻：破損中は激しいショートスパーク（白と赤の超高速点滅）
	if (outerShell_) {
		float shimmer = (0.90f + 0.10f * std::sin(visualTime_ * 3.0f)) * matGlow;
		Vector4 c;
		if (outerHealth < 1.0f) {
			float strobe = (std::sin(visualTime_ * 48.0f) > 0.0f) ? 1.0f : 0.2f;
			c = Vector4{ 1.0f, 0.2f * outerHealth + 0.8f * strobe, 0.2f * outerHealth + 0.8f * strobe, outerProg * outerHealth };
		} else {
			c = Vector4{ 1.0f * shimmer, 1.0f * shimmer, 1.0f * shimmer, outerProg };
		}
		outerShell_->SetColor(c);
	}
	// 中間層：破損中はマゼンタ過負荷・プラズマストロボ
	if (middleShell_) {
		float shimmer = (0.85f + 0.15f * std::cos(visualTime_ * 3.0f)) * matGlow;
		Vector4 c;
		if (midHealth < 1.0f) {
			float strobe = (std::sin(visualTime_ * 42.0f) > 0.0f) ? 1.0f : 0.1f;
			c = Vector4{ 1.0f, 0.1f * midHealth + 0.9f * strobe, 0.7f * midHealth, 0.90f * midProg * midHealth };
		} else {
			c = Vector4{ 1.0f * shimmer, 0.05f * shimmer, 0.80f * shimmer, 0.85f * midProg };
		}
		middleShell_->SetColor(c);
	}
	if (gyroRingX_) {
		float strobe = (midHealth < 1.0f && std::sin(visualTime_ * 36.0f) > 0.0f) ? 1.5f : 1.0f;
		gyroRingX_->SetColor({0.3f * matGlow * strobe, 0.9f * matGlow * strobe, 1.0f * matGlow * strobe, 0.85f * midProg * midHealth});
	}
	if (gyroRingY_) {
		float strobe = (midHealth < 1.0f && std::sin(visualTime_ * 36.0f) < 0.0f) ? 1.5f : 1.0f;
		gyroRingY_->SetColor({1.0f * matGlow * strobe, 0.3f * matGlow * strobe, 0.9f * matGlow * strobe, 0.85f * midProg * midHealth});
	}
	// 中心核：臨界時は超高輝度プラズマゴールド＆ホワイトの限界ストロボ発光
	if (centerCore_) {
		if (coreHealth < 1.0f) {
			float strobe = (std::sin(visualTime_ * 55.0f) > 0.0f) ? 2.0f : 0.4f;
			centerCore_->SetColor({1.0f * strobe, 0.95f * strobe, 0.4f * strobe, coreProg * coreHealth});
		} else {
			float pulse = (0.85f + 0.15f * std::sin(visualTime_ * 5.0f)) * matGlow;
			centerCore_->SetColor({1.0f * pulse, 0.92f * pulse, 0.20f * pulse, coreProg * coreHealth});
		}
	}
	// シアン光核
	if (cyanCore_) {
		cyanCore_->SetColor({0.0f, 1.0f * matGlow, 1.0f * matGlow, coreProg * coreHealth});
	}
	// マゼンタ光核
	if (magentaCore_) {
		magentaCore_->SetColor({1.0f * matGlow, 0.0f, 0.85f * matGlow, coreProg * coreHealth});
	}
	// 前方ポインター
	if (forwardPointer_) {
		forwardPointer_->SetColor({1.0f * matGlow, 0.90f * matGlow, 0.30f * matGlow, 0.95f * outerProg * outerHealth});
	}
}

/// <summary>
/// 幾何学多面体パーツの描画（段階的崩壊に対応）
/// </summary>
void Player::DrawGeometricVisual() {
	if (materializeProgress_ <= 0.001f) return;

	float coreProg = (std::clamp)((materializeProgress_ - 0.0f) / 0.40f, 0.0f, 1.0f);
	float midProg  = (std::clamp)((materializeProgress_ - 0.25f) / 0.45f, 0.0f, 1.0f);
	float outerProg= (std::clamp)((materializeProgress_ - 0.50f) / 0.50f, 0.0f, 1.0f);

	// 破壊・ディゾルブによる段階別ヘルス (1.0 -> 0.0)
	float outerHealth = 1.0f;
	float midHealth   = 1.0f;
	float coreHealth  = 1.0f;
	if (useDissolve_ || dissolveThreshold_ > 0.0f) {
		outerHealth = (std::clamp)(1.0f - (dissolveThreshold_ - 0.00f) / 0.35f, 0.0f, 1.0f);
		midHealth   = (std::clamp)(1.0f - (dissolveThreshold_ - 0.30f) / 0.38f, 0.0f, 1.0f);
		coreHealth  = (std::clamp)(1.0f - (dissolveThreshold_ - 0.65f) / 0.35f, 0.0f, 1.0f);
	}

	// 1. 内側コア（シアン＆マゼンタ＆中心ビーコン）: 0.65〜1.00まで残存
	if (coreProg > 0.001f && coreHealth > 0.001f) {
		if (cyanCore_) cyanCore_->Draw();
		if (magentaCore_) magentaCore_->Draw();
		if (centerCore_) centerCore_->Draw();
	}

	// 2. 中間層（十二面体マゼンタ＆ジャイロリング）: 0.0〜0.68まで残存
	if (midProg > 0.001f && midHealth > 0.001f) {
		if (middleShell_) middleShell_->Draw();
		if (gyroRingX_) gyroRingX_->Draw();
		if (gyroRingY_) gyroRingY_->Draw();
	}

	// 3. 外殻（二十面体）＆前方ポインター: 0.0〜0.35で真っ先に崩壊
	if (outerProg > 0.001f && outerHealth > 0.001f) {
		if (outerShell_) outerShell_->Draw();
		if (forwardPointer_) forwardPointer_->Draw();
	}
}



