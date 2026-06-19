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

		// 定数配列からディレイと最大スケールを適用
		rings_[i].delay = kRingDelays[i];
		rings_[i].maxScale = kRingMaxScales[i];

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
		billboardMatrix.m[kRowTranslation][kColX] = kZero;
		billboardMatrix.m[kRowTranslation][kColY] = kZero;
		billboardMatrix.m[kRowTranslation][kColZ] = kZero;
		billboardMatrix.m[kRowTranslation][kColW] = kMatrixTranslationW;


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
