#include "Shake.h"
#include "Random.h"

void Shake::Start(float duration, float intensity) {
	duration_ = duration;
	timer_ = duration;
	intensity_ = intensity;
	isActive_ = true;
	offset_ = { 0.0f, 0.0f, 0.0f };
}

void Shake::Update(float deltaTime) {
	if (!isActive_) {
		offset_ = { 0.0f, 0.0f, 0.0f };
		return;
	}

	timer_ -= deltaTime;
	if (timer_ <= 0.0f) {
		timer_ = 0.0f;
		isActive_ = false;
		offset_ = { 0.0f, 0.0f, 0.0f };
		return;
	}

	// 残り時間に応じて揺れの強さを減衰させる (線形減衰)
	float currentIntensity = intensity_ * (timer_ / duration_);

	// ランダムな方向ベクトルを生成し、現在の強さを掛ける
	offset_.x = Random::RangeFloat(-1.0f, 1.0f) * currentIntensity;
	offset_.y = Random::RangeFloat(-1.0f, 1.0f) * currentIntensity;
	offset_.z = Random::RangeFloat(-1.0f, 1.0f) * currentIntensity;
}
