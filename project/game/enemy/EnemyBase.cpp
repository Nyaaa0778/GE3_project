#include "EnemyBase.h"
#include "Object3d.h"
#include "TimeManager.h"
#include "Random.h"
#include "ParticleManager.h"
#include <cassert>
#include <array>

void EnemyBase::Initialize(Object3d* model, Camera* camera, const Vector3& pos) {
	assert(model);
	model_ = model;
	camera_ = camera;

	// トランスフォームの初期化
	worldTransform_.Initialize();
	worldTransform_.translation = pos;
	baseScale_ = worldTransform_.scale;

	state_ = EnemyState::kAlive;
	hasGivenScore_ = false;
	deathTimer_ = 0.0f;
	glitchFrameCount_ = 0;

	// モデルにカメラをセット
	model_->SetCamera(camera_);
}

void EnemyBase::Update() {
	if (state_ == EnemyState::kDying) {
		UpdateDeath();
		return;
	}

	worldTransform_.UpdateMatrix();
	model_->Update(&worldTransform_);
}

void EnemyBase::UpdateDeath() {
	float dt = TimeManager::GetInstance()->GetDeltaTime();
	deathTimer_ += dt;
	glitchFrameCount_++;

	float progress = deathTimer_ / kDeathDuration;
	if (progress >= 1.0f) {
		state_ = EnemyState::kDead;
		return;
	}

	// 1. 位置ジッター（RGBズレ・ブレ表現）
	float jitterAmp = (1.0f - progress * 0.5f) * 0.25f;
	worldTransform_.translation = {
		deathBasePos_.x + Random::RangeFloat(-jitterAmp, jitterAmp),
		deathBasePos_.y + Random::RangeFloat(-jitterAmp, jitterAmp),
		deathBasePos_.z + Random::RangeFloat(-jitterAmp, jitterAmp)
	};

	// 2. スケールの走査線変形（Y軸が急速に薄くなり、X/Zがノイズ伸縮）
	float scaleY = baseScale_.y * (1.0f - progress);
	float scaleNoiseX = 1.0f + Random::RangeFloat(-0.3f, 0.4f);
	float scaleNoiseZ = 1.0f + Random::RangeFloat(-0.3f, 0.4f);
	worldTransform_.scale = {
		baseScale_.x * scaleNoiseX,
		scaleY,
		baseScale_.z * scaleNoiseZ
	};

	// 3. デジタルブロック（キューブ）粒子の放出
	int emitCount = Random::RangeInt(1, 3);
	for (int i = 0; i < emitCount; ++i) {
		Vector3 particlePos = {
			deathBasePos_.x + Random::RangeFloat(-baseScale_.x * 0.6f, baseScale_.x * 0.6f),
			deathBasePos_.y + Random::RangeFloat(-baseScale_.y * 0.6f, baseScale_.y * 0.6f),
			deathBasePos_.z + Random::RangeFloat(-baseScale_.z * 0.6f, baseScale_.z * 0.6f)
		};
		// 四方へ拡散しつつ少し上へ浮遊
		Vector3 vel = {
			Random::RangeFloat(-3.5f, 3.5f),
			Random::RangeFloat(1.0f, 4.0f),
			Random::RangeFloat(-3.5f, 3.5f)
		};
		// シアン、マゼンタ、ホワイト、ネオンブルー
		static const std::array<Vector4, 4> kGlitchColors = {
			Vector4{ 0.0f, 1.0f, 1.0f, 1.0f }, // シアン
			Vector4{ 1.0f, 0.0f, 0.8f, 1.0f }, // マゼンタ
			Vector4{ 1.0f, 1.0f, 1.0f, 1.0f }, // 白
			Vector4{ 0.2f, 0.8f, 1.0f, 1.0f }  // ネオンブルー
		};
		Vector4 color = kGlitchColors[Random::RangeInt(0, static_cast<int>(kGlitchColors.size()) - 1)];
		float boxSize = Random::RangeFloat(0.12f, 0.25f);
		Vector3 pScale = { boxSize, boxSize, boxSize };
		float lifeTime = Random::RangeFloat(0.3f, 0.6f);

		ParticleManager::GetInstance()->Emit("DigitalGlitchBox", particlePos, vel, color, pScale, lifeTime, 1);
	}

	worldTransform_.UpdateMatrix();
	model_->Update(&worldTransform_);
}

void EnemyBase::Draw() {
	if (state_ == EnemyState::kDead) {
		return;
	}

	if (state_ == EnemyState::kDying) {
		// グリッチ点滅カラーの適用
		static const std::array<Vector4, 5> kFlickerColors = {
			Vector4{ 0.0f, 1.0f, 1.0f, 1.0f }, // シアン
			Vector4{ 1.0f, 0.1f, 0.7f, 1.0f }, // マゼンタ
			Vector4{ 1.0f, 1.0f, 1.0f, 1.0f }, // 純白
			Vector4{ 1.0f, 1.0f, 0.2f, 1.0f }, // イエロー
			Vector4{ 0.0f, 0.8f, 1.0f, 1.0f }  // ネオンブルー
		};

		Vector4 originalColor = model_->GetColor();
		Vector4 glitchColor = kFlickerColors[glitchFrameCount_ % kFlickerColors.size()];

		model_->SetColor(glitchColor);
		model_->Draw(&worldTransform_);
		model_->SetColor(originalColor);
		return;
	}

	model_->Draw(&worldTransform_);
}

void EnemyBase::OnCollision() {
	if (state_ != EnemyState::kAlive) return;

	// 被弾時に死亡演出（kDying）に移行
	state_ = EnemyState::kDying;
	deathTimer_ = 0.0f;
	glitchFrameCount_ = 0;
	deathBasePos_ = worldTransform_.GetWorldPosition();
	baseScale_ = worldTransform_.scale;
}

Vector3 EnemyBase::GetWorldPosition() {
	return worldTransform_.GetWorldPosition();
}

