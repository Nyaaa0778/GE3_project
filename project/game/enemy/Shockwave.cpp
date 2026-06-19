#include "Shockwave.h"
#include "Plane.h"
#include "Camera.h"
#include "MathUtility.h"
#include <cmath>

Shockwave::Shockwave() = default;
Shockwave::~Shockwave() = default;

void Shockwave::Initialize(Camera* camera, const Vector3& position) {
	camera_ = camera;
	position_ = position;
	
	// 設定したリング数分確保
	rings_.resize(kRingCount);

	for (size_t i = 0; i < kRingCount; ++i) {
		rings_[i].plane = std::make_unique<Plane>();
		rings_[i].plane->Initialize(kTextureName);
		rings_[i].plane->SetCamera(camera);
		
		// 外部トランスフォームとして紐付け
		rings_[i].plane->SetWorldTransform(&rings_[i].worldTransform);
		rings_[i].plane->SetBlendMode(PrimitiveRenderer::BlendMode::kAdd);

		// ループのインデックスからディレイとスケールを計算（マジックナンバー排除）
		rings_[i].delay = static_cast<float>(i) * kDelayInterval;
		
		// 等差でスケールを減衰させる (i=0 -> 3.0f, i=1 -> 2.4f, i=2 -> 1.8f)
		float scaleDiff = kBaseMaxScale * (kOne - kScaleDecreaseRatio);
		rings_[i].maxScale = kBaseMaxScale - static_cast<float>(i) * scaleDiff;
	}
}

void Shockwave::Update() {
	for (auto& ring : rings_) {
		if (ring.isFinished) continue;

		// 毎フレーム一定の時間を進める
		ring.elapsed += kFrameTime;
		if (ring.elapsed < ring.delay) {
			// 遅延時間中の場合、スケールをゼロにして更新だけ行う
			ring.worldTransform.matWorld = MathUtility::MakeScaleMatrix({kZero, kZero, kZero});
			ring.plane->Update();
			continue;
		}

		// アニメーションの進捗率 (0.0f ～ 1.0f)
		float t = (ring.elapsed - ring.delay) / kRingDuration;
		if (t >= kOne) {
			t = kOne;
			ring.isFinished = true;
		}

		// イージング（Ease-out）による滑らかな拡大
		float easeOut = kOne - std::pow(kOne - t, kEaseOutPower);
		float currentScale = easeOut * ring.maxScale;

		// ビルボード行列の計算
		Matrix4x4 billboardMatrix = camera_->GetWorldMatrix();
		billboardMatrix.m[3][0] = kZero;
		billboardMatrix.m[3][1] = kZero;
		billboardMatrix.m[3][2] = kZero;
		billboardMatrix.m[3][3] = kMatrixTranslationW;

		Matrix4x4 scaleMatrix = MathUtility::MakeScaleMatrix({ currentScale, currentScale, kScaleZDefault });
		Matrix4x4 translateMatrix = MathUtility::MakeTranslateMatrix(position_);

		// アフィン変換行列の合成
		ring.worldTransform.matWorld = MathUtility::Multiply(MathUtility::Multiply(scaleMatrix, billboardMatrix), translateMatrix);

		// アルファ値（透明度）の「ほわほわん」としたコントロール：
		float alpha = kZero;
		if (t < kFadeInDurationRatio) {
			alpha = t / kFadeInDurationRatio;
		} else {
			alpha = kOne - (t - kFadeInDurationRatio) / kFadeOutDurationRatio;
		}

		ring.plane->SetColor({ kColorRed, kColorGreen, kColorBlue, alpha });
		ring.plane->Update();
	}
}

void Shockwave::Draw() {
	for (const auto& ring : rings_) {
		if (ring.elapsed >= ring.delay && !ring.isFinished) {
			ring.plane->Draw();
		}
	}
}

bool Shockwave::IsFinished() const {
	for (const auto& ring : rings_) {
		if (!ring.isFinished) {
			return false;
		}
	}
	return true;
}
