#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "CityBackground.h"
#include "WireframeObject.h"
#include "Camera.h"
#include <MathUtility.h>
#include <cmath>
#include <random>
#include <algorithm>

namespace {
	// Catmull-Rom スプライン補間ヘルパー
	Vector3 CatmullRomSpline(const std::vector<Vector3>& points, size_t index, float t) {
		size_t n = points.size();
		if (n == 0) return {0.0f, 0.0f, 0.0f};
		if (n == 1) return points[0];

		size_t i0 = (index == 0) ? 0 : index - 1;
		size_t i1 = index;
		size_t i2 = (std::min)(n - 1, index + 1);
		size_t i3 = (std::min)(n - 1, index + 2);

		const Vector3& p0 = points[i0];
		const Vector3& p1 = points[i1];
		const Vector3& p2 = points[i2];
		const Vector3& p3 = points[i3];

		float t2 = t * t;
		float t3 = t2 * t;

		Vector3 a = MathUtility::Multiply(p1, 2.0f);
		Vector3 b = MathUtility::Multiply(MathUtility::Subtract(p2, p0), t);
		Vector3 c = MathUtility::Multiply(
			MathUtility::Add(MathUtility::Subtract(MathUtility::Multiply(p0, 2.0f), MathUtility::Multiply(p1, 5.0f)),
				MathUtility::Subtract(MathUtility::Multiply(p2, 4.0f), p3)),
			t2);
		Vector3 d = MathUtility::Multiply(
			MathUtility::Add(MathUtility::Subtract(MathUtility::Multiply(p0, -1.0f), MathUtility::Multiply(p2, 3.0f)),
				MathUtility::Add(MathUtility::Multiply(p1, 3.0f), p3)),
			t3);

		return MathUtility::Multiply(MathUtility::Add(MathUtility::Add(a, b), MathUtility::Add(c, d)), 0.5f);
	}

	Vector3 CatmullRomTangent(const std::vector<Vector3>& points, size_t index, float t) {
		size_t n = points.size();
		if (n == 0) return {0.0f, 0.0f, 1.0f};
		if (n == 1) return {0.0f, 0.0f, 1.0f};

		size_t i0 = (index == 0) ? 0 : index - 1;
		size_t i1 = index;
		size_t i2 = (std::min)(n - 1, index + 1);
		size_t i3 = (std::min)(n - 1, index + 2);

		const Vector3& p0 = points[i0];
		const Vector3& p1 = points[i1];
		const Vector3& p2 = points[i2];
		const Vector3& p3 = points[i3];

		float t2 = t * t;

		Vector3 a = MathUtility::Subtract(p2, p0);
		Vector3 b = MathUtility::Multiply(
			MathUtility::Add(MathUtility::Subtract(MathUtility::Multiply(p0, 2.0f), MathUtility::Multiply(p1, 5.0f)),
				MathUtility::Subtract(MathUtility::Multiply(p2, 4.0f), p3)),
			2.0f * t);
		Vector3 c = MathUtility::Multiply(
			MathUtility::Add(MathUtility::Subtract(MathUtility::Multiply(p0, -1.0f), MathUtility::Multiply(p2, 3.0f)),
				MathUtility::Add(MathUtility::Multiply(p1, 3.0f), p3)),
			3.0f * t2);

		return MathUtility::Multiply(MathUtility::Add(MathUtility::Add(a, b), c), 0.5f);
	}

	Vector3 EvaluateSpline(const std::vector<Vector3>& points, float time) {
		size_t n = points.size();
		if (n == 0) return {0.0f, 0.0f, 0.0f};
		if (n == 1) return points[0];

		size_t segmentIndex = static_cast<size_t>(time);
		float t = 0.0f;
		if (segmentIndex >= n - 1) {
			segmentIndex = n - 2;
			t = 1.0f;
		} else {
			t = time - static_cast<float>(segmentIndex);
		}
		return CatmullRomSpline(points, segmentIndex, t);
	}

	Vector3 EvaluateSplineTangent(const std::vector<Vector3>& points, float time) {
		size_t n = points.size();
		if (n == 0) return {0.0f, 0.0f, 1.0f};
		if (n == 1) return {0.0f, 0.0f, 1.0f};

		size_t segmentIndex = static_cast<size_t>(time);
		float t = 0.0f;
		if (segmentIndex >= n - 1) {
			segmentIndex = n - 2;
			t = 1.0f;
		} else {
			t = time - static_cast<float>(segmentIndex);
		}
		return CatmullRomTangent(points, segmentIndex, t);
	}

	struct SplineSample {
		Vector3 pos;
		Vector3 tangent;
		float dist = 0.0f;
	};
}

CityBackground::CityBackground() = default;
CityBackground::~CityBackground() = default;

void CityBackground::Initialize(Camera* camera, float minZ, float maxZ, int numBuildingsPerSide, float roadHalfWidth, float gridLength, float fogNear, float fogFar) {
	camera_ = camera;
	minZ_ = minZ;
	maxZ_ = maxZ;
	numBuildingsPerSide_ = numBuildingsPerSide;
	roadHalfWidth_ = roadHalfWidth;
	gridLength_ = gridLength;
	fogNear_ = fogNear;
	fogFar_ = fogFar;

	// 1. 地面のネオングリッド生成 (奥まで視界をカバーする幅と奥行き長めの長方形マス)
	float gridWidth = (std::max)(140.0f, roadHalfWidth_ * 6.0f + 60.0f);
	uint32_t divX = 24; // 横幅分割
	uint32_t divZ = 24; // 奥行き分割

	groundGrid1_ = std::make_unique<WireframeObject>();
	groundGrid1_->Initialize();
	groundGrid1_->SetCamera(camera_);
	groundGrid1_->CreateGrid(gridWidth, gridLength_, divX, divZ);
	groundGrid1_->SetPosition({0.0f, baseY_, minZ_ + gridLength_ * 0.5f});
	groundGrid1_->SetColor({0.0f, 0.40f, 0.50f, 0.70f}); // 落ち着いたディープシアン
	groundGrid1_->SetFog(fogNear_, fogFar_);

	groundGrid2_ = std::make_unique<WireframeObject>();
	groundGrid2_->Initialize();
	groundGrid2_->SetCamera(camera_);
	groundGrid2_->CreateGrid(gridWidth, gridLength_, divX, divZ);
	groundGrid2_->SetPosition({0.0f, baseY_, minZ_ + gridLength_ * 1.5f});
	groundGrid2_->SetColor({0.0f, 0.40f, 0.50f, 0.70f});
	groundGrid2_->SetFog(fogNear_, fogFar_);

	// 3. サイバーパンクビル群の生成
	GenerateCity();
}

void CityBackground::InitializeAlongPath(Camera* camera, const std::vector<Vector3>& pathPoints, float roadHalfWidth, float buildingInterval, float extraEndMargin, float fogNear, float fogFar) {
	camera_ = camera;
	roadHalfWidth_ = roadHalfWidth;
	scrollSpeed_ = 0.0f; // レールに沿って静止配置
	fogNear_ = fogNear;
	fogFar_ = fogFar;

	if (pathPoints.size() < 2) {
		// 制御点が足りない場合は直線配置へフォールバック
		Initialize(camera, -20.0f, 320.0f, 65, roadHalfWidth, 200.0f, fogNear_, fogFar_);
		return;
	}

	buildings_.clear();

	// 1. スプラインのアーク長サンプリングテーブルを構築
	const size_t numSegments = pathPoints.size() - 1;
	const size_t samplesPerSegment = 25;
	const size_t totalSamples = numSegments * samplesPerSegment;

	std::vector<SplineSample> samples;
	samples.reserve(totalSamples + 1);

	float accumulatedDist = 0.0f;
	Vector3 prevPos = pathPoints.front();

	float minX = prevPos.x, maxX = prevPos.x;
	float minZ = prevPos.z, maxZ = prevPos.z;

	for (size_t i = 0; i <= totalSamples; ++i) {
		float splineTime = static_cast<float>(i) / static_cast<float>(totalSamples) * static_cast<float>(numSegments);
		Vector3 p = EvaluateSpline(pathPoints, splineTime);
		Vector3 t = EvaluateSplineTangent(pathPoints, splineTime);

		if (i > 0) {
			float segmentDist = MathUtility::Length(MathUtility::Subtract(p, prevPos));
			accumulatedDist += segmentDist;
		}

		minX = (std::min)(minX, p.x);
		maxX = (std::max)(maxX, p.x);
		minZ = (std::min)(minZ, p.z);
		maxZ = (std::max)(maxZ, p.z);

		SplineSample sample;
		sample.pos = p;
		sample.tangent = t;
		sample.dist = accumulatedDist;
		samples.push_back(sample);

		prevPos = p;
	}

	const float totalCourseLength = accumulatedDist;

	// 距離 s から位置と接線を取得するラムダ式
	auto GetPointAtDistance = [&](float s, Vector3& outPos, Vector3& outTangent) {
		if (s <= 0.0f) {
			// 始点手前への直線延長
			Vector3 tan0 = samples.front().tangent;
			float len = MathUtility::Length(tan0);
			if (len > 0.0001f) {
				tan0 = MathUtility::Multiply(tan0, 1.0f / len);
			} else {
				tan0 = {0.0f, 0.0f, 1.0f};
			}
			outPos = MathUtility::Add(samples.front().pos, MathUtility::Multiply(tan0, s));
			outTangent = tan0;
		} else if (s >= totalCourseLength) {
			// ゴール地点の先への直線延長
			Vector3 tanLast = samples.back().tangent;
			float len = MathUtility::Length(tanLast);
			if (len > 0.0001f) {
				tanLast = MathUtility::Multiply(tanLast, 1.0f / len);
			} else {
				tanLast = {0.0f, 0.0f, 1.0f};
			}
			outPos = MathUtility::Add(samples.back().pos, MathUtility::Multiply(tanLast, s - totalCourseLength));
			outTangent = tanLast;
		} else {
			// スプライン上の2サンプル間を線形補間
			auto it = std::lower_bound(samples.begin(), samples.end(), s, [](const SplineSample& a, float val) {
				return a.dist < val;
			});
			if (it == samples.begin()) {
				outPos = it->pos;
				outTangent = it->tangent;
			} else {
				auto itPrev = it - 1;
				float delta = it->dist - itPrev->dist;
				float factor = (delta > 0.0001f) ? (s - itPrev->dist) / delta : 0.0f;
				factor = (std::max)(0.0f, (std::min)(1.0f, factor));

				outPos = MathUtility::Add(itPrev->pos, MathUtility::Multiply(MathUtility::Subtract(it->pos, itPrev->pos), factor));
				outTangent = MathUtility::Add(itPrev->tangent, MathUtility::Multiply(MathUtility::Subtract(it->tangent, itPrev->tangent), factor));
			}
		}
	};

	// 2. 地面グリッドの生成 (コース全体およびゴール地点をすっぽり覆う広大なサイバーグリッド)
	float spanX = (maxX - minX) + (roadHalfWidth_ + 60.0f) * 2.0f;
	spanX = (std::max)(spanX, 140.0f);
	float spanZ = (maxZ - minZ) + extraEndMargin + 80.0f;
	spanZ = (std::max)(spanZ, 180.0f);

	float centerX = (minX + maxX) * 0.5f;
	float centerZ = (minZ + maxZ) * 0.5f + (extraEndMargin * 0.25f);

	uint32_t divX = (std::max)(14u, static_cast<uint32_t>(spanX / 6.0f));
	uint32_t divZ = (std::max)(14u, static_cast<uint32_t>(spanZ / 18.0f)); // 奥行き長めマスでスピード感

	groundGrid1_ = std::make_unique<WireframeObject>();
	groundGrid1_->Initialize();
	groundGrid1_->SetCamera(camera_);
	groundGrid1_->CreateGrid(spanX, spanZ, divX, divZ);
	groundGrid1_->SetPosition({centerX, baseY_, centerZ});
	groundGrid1_->SetColor({0.0f, 0.40f, 0.50f, 0.70f}); // 落ち着いたディープシアン
	groundGrid1_->SetFog(fogNear_, fogFar_);

	groundGrid2_.reset(); // コース全体を1枚の広大なグリッドでカバーするため2枚目はリセット

	// 3. コース沿いにゴール地点までビル群を生成
	std::mt19937 rng(42);
	std::uniform_real_distribution<float> randDist(-1.0f, 1.0f);
	std::uniform_real_distribution<float> rand01(0.0f, 1.0f);

	// カラーパレット: 背景として手前を邪魔しない落ち着いたダークネオントーン
	const Vector4 kColorCyan    = {0.0f, 0.45f, 0.55f, 0.75f};  // 落ち着いたダークシアン
	const Vector4 kColorMagenta = {0.55f, 0.03f, 0.40f, 0.75f}; // 落ち着いたダークマゼンタ

	float startDist = -15.0f; // スタート手前からビルを配置
	float endDist = totalCourseLength + extraEndMargin; // ゴール地点を越えて余白まで配置

	for (float s = startDist; s <= endDist; s += buildingInterval) {
		// わずかなジッターで規則的な並びを自然に崩す
		float jitteredDist = s + randDist(rng) * (buildingInterval * 0.2f);

		Vector3 centerPos, tangent;
		GetPointAtDistance(jitteredDist, centerPos, tangent);

		// 水平面での接線 (XZ) と右方向ベクトル
		float tanLenXZ = std::sqrt(tangent.x * tangent.x + tangent.z * tangent.z);
		Vector3 tangentXZ = (tanLenXZ > 0.0001f) ? Vector3{tangent.x / tanLenXZ, 0.0f, tangent.z / tanLenXZ} : Vector3{0.0f, 0.0f, 1.0f};
		Vector3 right = { tangentXZ.z, 0.0f, -tangentXZ.x };

		// ビルの回転角 (道路の向きに沿わせる)
		float yaw = std::atan2(tangentXZ.x, tangentXZ.z);

		// 左右両脇に配置 (-1: 左側, +1: 右側)
		for (int side = -1; side <= 1; side += 2) {
			// === Layer 1: 道路沿いのビル ===
			{
				Building b;
				float w = 6.0f + rand01(rng) * 10.0f; // 幅 6m〜16m
				float d = 6.0f + rand01(rng) * 10.0f; // 奥行き 6m〜16m
				float h = 8.0f + std::pow(rand01(rng), 1.8f) * 32.0f; // 高さ 8m〜40m

				float offsetDist = roadHalfWidth_ + w * 0.5f + rand01(rng) * 3.0f;
				Vector3 bPos = MathUtility::Add(centerPos, MathUtility::Multiply(right, static_cast<float>(side) * offsetDist));
				bPos.y = baseY_ + h * 0.5f;

				b.basePosition = bPos;
				b.scale = {w, h, d};

				float accentRoll = rand01(rng);
				if (h > 18.0f || accentRoll > 0.75f) {
					b.baseColor = kColorMagenta;
				} else {
					b.baseColor = kColorCyan;
				}

				b.pulsePhase = rand01(rng) * 6.28318f;
				b.pulseSpeed = 1.0f + rand01(rng) * 2.0f;

				b.wireObject = std::make_unique<WireframeObject>();
				b.wireObject->Initialize();
				b.wireObject->SetCamera(camera_);
				b.wireObject->CreateBox({1.0f, 1.0f, 1.0f});
				b.wireObject->SetScale(b.scale);
				b.wireObject->SetRotation({0.0f, yaw, 0.0f});
				b.wireObject->SetPosition(b.basePosition);
				b.wireObject->SetColor(b.baseColor);
				b.wireObject->SetFog(fogNear_, fogFar_);

				buildings_.push_back(std::move(b));
			}

			// === Layer 2: 外側の高層メガストラクチャー (約60%の確率で配置して都市の厚みを演出) ===
			if (rand01(rng) < 0.60f) {
				Building b2;
				float w2 = 8.0f + rand01(rng) * 12.0f; // 幅 8m〜20m
				float d2 = 8.0f + rand01(rng) * 12.0f;
				float h2 = 14.0f + std::pow(rand01(rng), 1.6f) * 36.0f; // より高層 (14m〜50m)

				float offsetDist2 = roadHalfWidth_ + 22.0f + rand01(rng) * 18.0f;
				Vector3 bPos2 = MathUtility::Add(centerPos, MathUtility::Multiply(right, static_cast<float>(side) * offsetDist2));
				bPos2.y = baseY_ + h2 * 0.5f;

				b2.basePosition = bPos2;
				b2.scale = {w2, h2, d2};

				float accentRoll = rand01(rng);
				if (h2 > 24.0f || accentRoll > 0.70f) {
					b2.baseColor = kColorMagenta;
				} else {
					b2.baseColor = kColorCyan;
				}

				b2.pulsePhase = rand01(rng) * 6.28318f;
				b2.pulseSpeed = 1.0f + rand01(rng) * 2.0f;

				b2.wireObject = std::make_unique<WireframeObject>();
				b2.wireObject->Initialize();
				b2.wireObject->SetCamera(camera_);
				b2.wireObject->CreateBox({1.0f, 1.0f, 1.0f});
				b2.wireObject->SetScale(b2.scale);
				b2.wireObject->SetRotation({0.0f, yaw, 0.0f});
				b2.wireObject->SetPosition(b2.basePosition);
				b2.wireObject->SetColor(b2.baseColor);
				b2.wireObject->SetFog(fogNear_, fogFar_);

				buildings_.push_back(std::move(b2));
			}
		}
	}
}


void CityBackground::GenerateCity() {
	buildings_.clear();

	std::mt19937 rng(42); // シードを固定して美しい配置を再現
	std::uniform_real_distribution<float> randDist(-1.0f, 1.0f);
	std::uniform_real_distribution<float> rand01(0.0f, 1.0f);

	// カラーパレット: 背景として手前を邪魔しない落ち着いたダークネオントーン
	const Vector4 kColorCyan    = {0.0f, 0.45f, 0.55f, 0.75f};  // 落ち着いたダークシアン
	const Vector4 kColorMagenta = {0.55f, 0.03f, 0.40f, 0.75f}; // 落ち着いたダークマゼンタ

	for (int side = -1; side <= 1; side += 2) { // -1: 左側, +1: 右側
		for (int i = 0; i < numBuildingsPerSide_; ++i) {
			// === Layer 1: 道路沿いのビル ===
			{
				Building b;

				// ビルのサイズ (幅・奥行きを大きめにして巨大なメガストラクチャー感を演出)
				float w = 6.0f + rand01(rng) * 10.0f; // 幅 6m〜16m
				float d = 6.0f + rand01(rng) * 10.0f; // 奥行き 6m〜16m
				float h = 8.0f + std::pow(rand01(rng), 1.8f) * 32.0f; // べき乗で低層多め・時々超高層 (8m〜40m)

				// X位置: 道路の外側に配置
				float layer = rand01(rng);
				float x = side * (roadHalfWidth_ + w * 0.5f + layer * 12.0f);

				// Z位置: 奥行き方向に均等配置＋適度なジッター
				float z = minZ_ + ((float)i / (float)numBuildingsPerSide_) * (maxZ_ - minZ_) + (randDist(rng) * 3.0f);

				// Y位置 (底面が地面 Y=baseY_ に接するよう高さの半分を加算)
				float y = baseY_ + h * 0.5f;

				b.basePosition = {x, y, z};
				b.scale = {w, h, d};

				// 配色の決定: 高層ビル(h>16m)または約25%の確率でアクセント(マゼンタ)、残りはシアンで統一
				float accentRoll = rand01(rng);
				if (h > 16.0f || accentRoll > 0.75f) {
					b.baseColor = kColorMagenta;
				} else {
					b.baseColor = kColorCyan;
				}

				b.pulsePhase = rand01(rng) * 6.28318f;
				b.pulseSpeed = 1.0f + rand01(rng) * 2.0f;

				b.wireObject = std::make_unique<WireframeObject>();
				b.wireObject->Initialize();
				b.wireObject->SetCamera(camera_);
				b.wireObject->CreateBox({1.0f, 1.0f, 1.0f}); // 単位立方体をスケールで拡大
				b.wireObject->SetScale(b.scale);
				b.wireObject->SetPosition(b.basePosition);
				b.wireObject->SetColor(b.baseColor);
				b.wireObject->SetFog(fogNear_, fogFar_);

				buildings_.push_back(std::move(b));
			}

			// === Layer 2: 外側の超高層メガストラクチャー (約55%の確率で配置し、摩天楼の奥行きを演出) ===
			if (rand01(rng) < 0.55f) {
				Building b2;

				float w2 = 8.0f + rand01(rng) * 14.0f;  // 幅 8m〜22m
				float d2 = 8.0f + rand01(rng) * 14.0f;  // 奥行き 8m〜22m
				float h2 = 16.0f + std::pow(rand01(rng), 1.6f) * 38.0f; // より高層 (16m〜54m)

				float x2 = side * (roadHalfWidth_ + 24.0f + rand01(rng) * 25.0f);
				float z2 = minZ_ + ((float)i / (float)numBuildingsPerSide_) * (maxZ_ - minZ_) + (randDist(rng) * 4.0f);
				float y2 = baseY_ + h2 * 0.5f;

				b2.basePosition = {x2, y2, z2};
				b2.scale = {w2, h2, d2};

				float accentRoll = rand01(rng);
				if (h2 > 24.0f || accentRoll > 0.70f) {
					b2.baseColor = kColorMagenta;
				} else {
					b2.baseColor = kColorCyan;
				}

				b2.pulsePhase = rand01(rng) * 6.28318f;
				b2.pulseSpeed = 1.0f + rand01(rng) * 2.0f;

				b2.wireObject = std::make_unique<WireframeObject>();
				b2.wireObject->Initialize();
				b2.wireObject->SetCamera(camera_);
				b2.wireObject->CreateBox({1.0f, 1.0f, 1.0f});
				b2.wireObject->SetScale(b2.scale);
				b2.wireObject->SetPosition(b2.basePosition);
				b2.wireObject->SetColor(b2.baseColor);
				b2.wireObject->SetFog(fogNear_, fogFar_);

				buildings_.push_back(std::move(b2));
			}
		}
	}
}

void CityBackground::Update() {
	time_ += 0.016f;

	// ビル群の段階的形成アニメーション
	if (buildAnimationEnabled_) {
		buildAnimTime_ += 0.016f;
		UpdateBuildAnimation();
	}

	// 1. 地面グリッドのスクロール
	if (scrollSpeed_ > 0.0f) {
		gridZ_ -= scrollSpeed_ * 0.016f;
		if (gridZ_ <= -gridLength_) {
			gridZ_ += gridLength_;
		}
	}
	if (groundGrid1_) {
		groundGrid1_->SetPosition({0.0f, baseY_, minZ_ + gridZ_ + gridLength_ * 0.5f});
		groundGrid1_->Update();
	}
	if (groundGrid2_) {
		groundGrid2_->SetPosition({0.0f, baseY_, minZ_ + gridZ_ + gridLength_ * 1.5f});
		groundGrid2_->Update();
	}

	// 3. ビル群のスクロールとネオン明滅アニメーション
	for (auto& b : buildings_) {
		// スクロール移動 (速度が0より大きい場合のみ)
		if (scrollSpeed_ > 0.0f) {
			b.basePosition.z -= scrollSpeed_ * 0.016f;
			if (b.basePosition.z < minZ_) {
				b.basePosition.z += (maxZ_ - minZ_);
			}
			b.wireObject->SetPosition(b.basePosition);
		}

		// ネオンの呼吸（穏やかなパルス発光のみ、不快なチカチカ点滅は削除）
		if (enablePulse_) {
			float pulse = 0.85f + 0.15f * std::sin(time_ * b.pulseSpeed + b.pulsePhase);
			Vector4 currentColor = {
				b.baseColor.x * pulse,
				b.baseColor.y * pulse,
				b.baseColor.z * pulse,
				b.baseColor.w
			};
			b.wireObject->SetColor(currentColor);
		}

		b.wireObject->Update();
	}
}

void CityBackground::Draw() {
	// 地面
	if (groundGrid1_) groundGrid1_->Draw();
	if (groundGrid2_) groundGrid2_->Draw();

	// ビル群
	for (auto& b : buildings_) {
		if (b.wireObject) {
			b.wireObject->Draw();
		}
	}
}

void CityBackground::SetBaseY(float baseY) {
	float diffY = baseY - baseY_;
	baseY_ = baseY;

	for (auto& b : buildings_) {
		b.basePosition.y += diffY;
		if (b.wireObject) {
			b.wireObject->SetPosition(b.basePosition);
		}
	}
	if (groundGrid1_) {
		groundGrid1_->SetPosition({0.0f, baseY_, minZ_ + gridZ_ + gridLength_ * 0.5f});
	}
	if (groundGrid2_) {
		groundGrid2_->SetPosition({0.0f, baseY_, minZ_ + gridZ_ + gridLength_ * 1.5f});
	}
}

void CityBackground::SetCamera(Camera* camera) {
	camera_ = camera;
	if (groundGrid1_) groundGrid1_->SetCamera(camera);
	if (groundGrid2_) groundGrid2_->SetCamera(camera);
	for (auto& b : buildings_) {
		if (b.wireObject) {
			b.wireObject->SetCamera(camera);
		}
	}
}

void CityBackground::SetFog(float fogNear, float fogFar) {
	fogNear_ = fogNear;
	fogFar_ = fogFar;
	if (groundGrid1_) groundGrid1_->SetFog(fogNear_, fogFar_);
	if (groundGrid2_) groundGrid2_->SetFog(fogNear_, fogFar_);
	for (auto& b : buildings_) {
		if (b.wireObject) {
			b.wireObject->SetFog(fogNear_, fogFar_);
		}
	}
}

void CityBackground::SetBuildAnimationEnabled(bool enabled) {
	buildAnimationEnabled_ = enabled;
	if (buildAnimationEnabled_) {
		SetupBuildAnimation();
	}
}

void CityBackground::ResetBuildAnimation() {
	buildAnimTime_ = 0.0f;
	for (auto& b : buildings_) {
		b.growth.isComplete = false;
		if (b.wireObject) {
			b.wireObject->UpdateDynamicLines({});
		}
	}
}

namespace {
	// 直方体 (単位立方体) の8頂点
	const Vector3 kBoxCorners[8] = {
		{-0.5f, -0.5f, -0.5f}, // 0: 底面 手前左
		{ 0.5f, -0.5f, -0.5f}, // 1: 底面 手前右
		{ 0.5f, -0.5f,  0.5f}, // 2: 底面 奥右
		{-0.5f, -0.5f,  0.5f}, // 3: 底面 奥左
		{-0.5f,  0.5f, -0.5f}, // 4: 上面 手前左
		{ 0.5f,  0.5f, -0.5f}, // 5: 上面 手前右
		{ 0.5f,  0.5f,  0.5f}, // 6: 上面 奥右
		{-0.5f,  0.5f,  0.5f}, // 7: 上面 奥左
	};

	struct BoxEdgeDef { int u; int v; };
	const BoxEdgeDef kBoxEdges[12] = {
		{0, 1}, {1, 2}, {2, 3}, {3, 0}, // 0..3: 地面底面の4辺
		{0, 4}, {1, 5}, {2, 6}, {3, 7}, // 4..7: 垂直の柱4本
		{4, 5}, {5, 6}, {6, 7}, {7, 4}  // 8..11: 屋根天面の4辺
	};

	const int kBoxVertexEdges[8][3] = {
		{0, 3, 4},   // 0
		{0, 1, 5},   // 1
		{1, 2, 6},   // 2
		{2, 3, 7},   // 3
		{4, 8, 11},  // 4
		{5, 8, 9},   // 5
		{6, 9, 10},  // 6
		{7, 10, 11}, // 7
	};
}

void CityBackground::SetupBuildAnimation() {
	buildAnimTime_ = 0.0f;
	std::mt19937 rng(1337);
	std::uniform_real_distribution<float> rand01(0.0f, 1.0f);
	std::uniform_int_distribution<int> randSeedVertex(0, 3);

	float zRange = (maxZ_ - minZ_ > 0.001f) ? (maxZ_ - minZ_) : 1.0f;

	for (auto& b : buildings_) {
		if (!b.wireObject) continue;

		// 12本の動的ラインメッシュに切り替え
		b.wireObject->CreateDynamicLineMesh(12);

		// 手前から奥へのウェーブ伝播＋ランダムジッターによる開始時差
		float zNorm = std::clamp((b.basePosition.z - minZ_) / zRange, 0.0f, 1.0f);
		b.growth.startDelay = zNorm * 2.2f + rand01(rng) * 1.5f;
		b.growth.isComplete = false;
		b.growth.edges.clear();
		b.growth.edges.reserve(12);

		// 地面の4角 (0〜3) のいずれかからスタート
		int seedVertex = randSeedVertex(rng);

		bool edgeScheduled[12] = {false};
		bool vertexReached[8] = {false};
		float vertexReachedTime[8] = {0.0f};

		vertexReached[seedVertex] = true;
		vertexReachedTime[seedVertex] = 0.0f;

		std::vector<int> frontier = {seedVertex};

		while (!frontier.empty()) {
			// 到達時刻が早い頂点から順に分岐を展開
			std::sort(frontier.begin(), frontier.end(), [&](int v1, int v2) {
				return vertexReachedTime[v1] < vertexReachedTime[v2];
			});
			int curr = frontier.front();
			frontier.erase(frontier.begin());

			// この頂点に繋がる3辺をランダム順にシャッフル
			std::vector<int> incident = {
				kBoxVertexEdges[curr][0],
				kBoxVertexEdges[curr][1],
				kBoxVertexEdges[curr][2]
			};
			std::shuffle(incident.begin(), incident.end(), rng);

			for (int edgeIdx : incident) {
				if (!edgeScheduled[edgeIdx]) {
					edgeScheduled[edgeIdx] = true;
					int nextVertex = (kBoxEdges[edgeIdx].u == curr) ? kBoxEdges[edgeIdx].v : kBoxEdges[edgeIdx].u;

					// 辺の伸長にかかる時間 (0.28秒〜0.46秒)
					float duration = 0.28f + rand01(rng) * 0.18f;
					// 分岐発生時の微小な時差ジッター
					float branchJitter = rand01(rng) * 0.06f;
					float edgeStart = vertexReachedTime[curr] + branchJitter;
					float edgeEnd = edgeStart + duration;

					EdgeGrowth eg;
					eg.fromVertex = curr;
					eg.toVertex = nextVertex;
					eg.startTime = edgeStart;
					eg.duration = duration;
					b.growth.edges.push_back(eg);

					if (!vertexReached[nextVertex]) {
						vertexReached[nextVertex] = true;
						vertexReachedTime[nextVertex] = edgeEnd;
						frontier.push_back(nextVertex);
					}
				}
			}
		}

		// 閉路を構成する残り辺があれば、両端点が到達された後に開始
		for (int e = 0; e < 12; ++e) {
			if (!edgeScheduled[e]) {
				edgeScheduled[e] = true;
				int u = kBoxEdges[e].u;
				int v = kBoxEdges[e].v;
				int fromV = (vertexReachedTime[u] <= vertexReachedTime[v]) ? u : v;
				int toV = (fromV == u) ? v : u;
				float startBase = (std::max)(vertexReachedTime[u], vertexReachedTime[v]);
				float edgeStart = startBase + rand01(rng) * 0.05f;
				float duration = 0.28f + rand01(rng) * 0.18f;

				EdgeGrowth eg;
				eg.fromVertex = fromV;
				eg.toVertex = toV;
				eg.startTime = edgeStart;
				eg.duration = duration;
				b.growth.edges.push_back(eg);
			}
		}

		// 初期状態は描画なし
		b.wireObject->UpdateDynamicLines({});
	}
}

void CityBackground::UpdateBuildAnimation() {
	std::vector<std::pair<Vector3, Vector3>> activeLines;
	activeLines.reserve(12);

	for (auto& b : buildings_) {
		if (b.growth.isComplete) {
			continue;
		}

		float localTime = buildAnimTime_ - b.growth.startDelay;
		if (localTime < 0.0f) {
			continue;
		}

		activeLines.clear();
		bool allEdgesDone = true;

		for (const auto& eg : b.growth.edges) {
			if (localTime < eg.startTime) {
				allEdgesDone = false;
				continue;
			}

			Vector3 p0 = kBoxCorners[eg.fromVertex];
			Vector3 p1 = kBoxCorners[eg.toVertex];

			if (localTime >= eg.startTime + eg.duration) {
				// 完了した辺
				activeLines.push_back({p0, p1});
			} else {
				// 現在伸長中の辺 (滑らかなEase-Out補間で先端が伸びる)
				allEdgesDone = false;
				float tNorm = (localTime - eg.startTime) / eg.duration;
				tNorm = (std::max)(0.0f, (std::min)(1.0f, tNorm));
				float progress = 1.0f - (1.0f - tNorm) * (1.0f - tNorm);
				Vector3 currentEnd = MathUtility::Add(p0, MathUtility::Multiply(MathUtility::Subtract(p1, p0), progress));
				activeLines.push_back({p0, currentEnd});
			}
		}

		if (allEdgesDone) {
			b.growth.isComplete = true;
			activeLines.clear();
			for (const auto& eg : b.growth.edges) {
				activeLines.push_back({kBoxCorners[eg.fromVertex], kBoxCorners[eg.toVertex]});
			}
		}

		if (b.wireObject) {
			b.wireObject->UpdateDynamicLines(activeLines);
		}
	}
}


