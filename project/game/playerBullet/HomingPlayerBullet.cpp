#include "HomingPlayerBullet.h"

#include <MathUtility.h>
#include <MyEngine.h>
#include "EnemyBase.h"
#include "Random.h"
#include "Easing.h"

#include <numbers>
#include <cmath>
#include <algorithm>

using namespace MathUtility;

void HomingPlayerBullet::Initialize(const PlayerBulletParam& param) {
	if (!model_) {
		model_ = std::make_unique<Object3d>();
		model_->Initialize("sphere");
	}
	model_->SetCamera(param.camera);

	worldTransform_.Initialize();
	worldTransform_.translation = param.position;
	worldTransform_.scale = {0.0f, 0.0f, 0.0f};
	worldTransform_.UpdateMatrix();

	startPos_ = param.position;
	prevWorldPos_ = param.position;
	target_ = param.target;
	if (target_ && target_->IsAlive()) {
		targetPos_ = target_->GetWorldPosition();
	} else {
		target_ = nullptr;
		Vector3 forward = Normalize(param.velocity);
		if (Length(forward) < 0.001f) {
			forward = { 0.0f, 0.0f, 1.0f };
		}
		targetPos_ = param.position + forward * 100.0f;
	}

	Vector3 toTarget = targetPos_ - startPos_;
	float dist = Length(toTarget);
	if (dist > 0.001f) {
		forwardDir_ = Normalize(toTarget);
	} else {
		forwardDir_ = { 0.0f, 0.0f, 1.0f };
	}

	// カメラから画面横方向（サイド方向）を取得して左右への弧の向きにする
	if (param.camera) {
		Matrix4x4 matView = param.camera->GetViewMatrix();
		Matrix4x4 matViewInv = MakeInverseMatrix(matView);
		sideDir_ = Normalize(Vector3(matViewInv.m[0][0], matViewInv.m[0][1], matViewInv.m[0][2]));
	} else {
		sideDir_ = { 1.0f, 0.0f, 0.0f };
	}

	// 左右ランダムに弧を描く符号（+1 または -1）
	curveSignX_ = (Random::RangeFloat(0.0f, 1.0f) > 0.5f) ? 1.0f : -1.0f;

	// 距離に応じて弧の幅を動的に設定
	arcWidth_ = std::clamp(dist * 0.2f, 2.5f, 7.0f) * Random::RangeFloat(0.9f, 1.1f);

	// 進行速度（高速に射出して自機より常に前方を先行）
	float duration = std::clamp(dist / 140.0f, 0.25f, 0.6f);
	progress_ = 0.0f;
	progressSpeed_ = 1.0f / duration;

	// コライダーの初期設定 (球, 半径 0.8)
	SetShape(ColliderShape::kSphere);
	SetSphere({ 0.8f });

	hasArrived_ = false;
	isDead_ = false;
}

void HomingPlayerBullet::Update(const std::list<EnemyBase*>& enemies) {
	if (isDead_) {
		return;
	}

	// 移動前のワールド座標を保存
	prevWorldPos_ = worldTransform_.translation;

	// ターゲットの生存確認
	if (target_) {
		auto it = std::find(enemies.begin(), enemies.end(), target_);
		if (it == enemies.end() || !target_->IsAlive()) {
			target_ = nullptr;
		} else {
			// 生存していればターゲットの最新座標を取得
			targetPos_ = target_->GetWorldPosition();
		}
	}

	float dt = TimeManager::GetInstance()->GetDeltaTime();
	progress_ += progressSpeed_ * dt;

	// 2次ベジェ曲線を用いた前方射出＋サイドカーブ軌道
	// P0: 自機位置（発射点）
	// P1: 自機の前方斜め外側に配置した制御点（必ず自機より前方へ射出される）
	// P2: ターゲット位置
	float dist = Length(targetPos_ - startPos_);
	Vector3 P0 = startPos_;
	Vector3 P1 = startPos_ + forwardDir_ * (dist * 0.5f) + sideDir_ * (arcWidth_ * curveSignX_);
	Vector3 P2 = targetPos_;

	// EaseIn を適用して、最初は緩やかに外側へ広がり、後半で敵へ向かって加速
	float u = Easing::EaseInQuad(std::clamp(progress_, 0.0f, 1.0f));

	float oneMinusU = 1.0f - u;
	Vector3 newPos = P0 * (oneMinusU * oneMinusU) + P1 * (2.0f * oneMinusU * u) + P2 * (u * u);

	// --- 確実なヒット判定 ---
	if (target_ && target_->IsAlive()) {
		Vector3 currentEnemyPos = target_->GetWorldPosition();
		float distToEnemy = Length(newPos - currentEnemyPos);

		// 進行完了(progress_ >= 1.0f) または 敵と十分接近(distToEnemy <= 2.5f)した場合、直撃確定
		if (progress_ >= 1.0f || distToEnemy <= 2.5f) {
			worldTransform_.translation = currentEnemyPos;
			worldTransform_.UpdateMatrix();
			model_->Update(&worldTransform_);

			// 着弾エフェクト用のパーティクル生成
			Vector4 hitColor = { 1.0f, 0.4f, 0.9f, 1.0f };
			Vector3 hitScale = { 1.2f, 1.2f, 1.2f };
			ParticleManager::GetInstance()->Emit("BulletTrail", currentEnemyPos, {0.0f, 0.0f, 0.0f}, hitColor, hitScale, 0.15f, 6);

			// 命中処理を実行
			OnCollision();
			target_->OnCollision();
			target_->SetKilledByPlayerBullet(true);
			return;
		}
	} else {
		// ターゲットが既に死亡または不在で、軌道終点に達した場合は消滅
		if (progress_ >= 1.0f) {
			isDead_ = true;
			return;
		}
	}

	worldTransform_.translation = newPos;
	worldTransform_.UpdateMatrix();
	model_->Update(&worldTransform_);

	// 光の軌道（パーティクルのトレイル）を生成
	Vector3 startStep = prevWorldPos_;
	Vector3 endStep = worldTransform_.translation;
	Vector3 diff = endStep - startStep;
	float totalDist = Length(diff);
	float stepDist = 0.25f;
	int numSteps = static_cast<int>(totalDist / stepDist);
	if (numSteps < 1) {
		numSteps = 1;
	}

	for (int i = 0; i < numSteps; ++i) {
		float t = static_cast<float>(i) / static_cast<float>(numSteps);
		Vector3 trailPos = startStep + diff * t;
		Vector4 trailColor = { 1.0f, 0.3f, 0.85f, 1.0f }; // 鮮やかなピンク・マゼンタ
		Vector3 trailScale = { 0.45f, 0.45f, 0.45f };
		float trailLifeTime = 0.08f; // 短寿命で残像が自機後方に残らないようにする
		ParticleManager::GetInstance()->Emit("BulletTrail", trailPos, {0.0f, 0.0f, 0.0f}, trailColor, trailScale, trailLifeTime, 1);
	}

	// 弾頭のコア（ピンクがかった白い高輝度パーティクル）
	Vector4 coreColor = { 1.0f, 0.85f, 0.95f, 1.0f };
	Vector3 coreScale = { 0.65f, 0.65f, 0.65f };
	float coreLifeTime = 0.05f;
	ParticleManager::GetInstance()->Emit("BulletTrail", endStep, {0.0f, 0.0f, 0.0f}, coreColor, coreScale, coreLifeTime, 1);
}
