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

void CityBackground::Initialize(Camera* camera, float minZ, float maxZ, int numBuildingsPerSide, float roadHalfWidth, float gridLength) {
	camera_ = camera;
	minZ_ = minZ;
	maxZ_ = maxZ;
	numBuildingsPerSide_ = numBuildingsPerSide;
	roadHalfWidth_ = roadHalfWidth;
	gridLength_ = gridLength;

	// 1. 地面のネオングリッド生成 (奥行き大きめの長方形マスでスピード感を演出)
	float gridWidth = (std::max)(70.0f, roadHalfWidth_ * 4.0f + 30.0f);
	uint32_t divX = 20; // 横幅分割
	uint32_t divZ = 16; // 奥行き分割 (横幅に対して奥行きが約2.5〜4倍長い長方形マス)

	groundGrid1_ = std::make_unique<WireframeObject>();
	groundGrid1_->Initialize();
	groundGrid1_->SetCamera(camera_);
	groundGrid1_->CreateGrid(gridWidth, gridLength_, divX, divZ);
	groundGrid1_->SetPosition({0.0f, baseY_, minZ_ + gridLength_ * 0.5f});
	groundGrid1_->SetColor({0.0f, 0.85f, 1.0f, 1.0f}); // ネオンシアン (不透明)

	groundGrid2_ = std::make_unique<WireframeObject>();
	groundGrid2_->Initialize();
	groundGrid2_->SetCamera(camera_);
	groundGrid2_->CreateGrid(gridWidth, gridLength_, divX, divZ);
	groundGrid2_->SetPosition({0.0f, baseY_, minZ_ + gridLength_ * 1.5f});
	groundGrid2_->SetColor({0.0f, 0.85f, 1.0f, 1.0f});

	// 3. サイバーパンクビル群の生成
	GenerateCity();
}

void CityBackground::InitializeAlongPath(Camera* camera, const std::vector<Vector3>& pathPoints, float roadHalfWidth, float buildingInterval, float extraEndMargin) {
	camera_ = camera;
	roadHalfWidth_ = roadHalfWidth;
	scrollSpeed_ = 0.0f; // レールに沿って静止配置

	if (pathPoints.size() < 2) {
		// 制御点が足りない場合は直線配置へフォールバック
		Initialize(camera, -20.0f, 200.0f, 40, roadHalfWidth, 220.0f);
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
	groundGrid1_->SetColor({0.0f, 0.85f, 1.0f, 1.0f}); // シアン

	groundGrid2_.reset(); // コース全体を1枚の広大なグリッドでカバーするため2枚目はリセット

	// 3. コース沿いにゴール地点までビル群を生成
	std::mt19937 rng(42);
	std::uniform_real_distribution<float> randDist(-1.0f, 1.0f);
	std::uniform_real_distribution<float> rand01(0.0f, 1.0f);

	// カラーパレット: シアン基調 (約75%) ＋ 高層アクセントにマゼンタ (約25%)、黄色なし
	const Vector4 kColorCyan    = {0.0f, 0.90f, 1.0f, 1.0f};
	const Vector4 kColorMagenta = {1.0f, 0.05f, 0.75f, 1.0f};

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

	// カラーパレット: シアン基調 (約75%) ＋ 高層アクセントにマゼンタ (約25%)
	const Vector4 kColorCyan    = {0.0f, 0.90f, 1.0f, 1.0f};  // ベース: ネオンシアン
	const Vector4 kColorMagenta = {1.0f, 0.05f, 0.75f, 1.0f}; // アクセント: ネオンマゼンタ

	for (int side = -1; side <= 1; side += 2) { // -1: 左側, +1: 右側
		for (int i = 0; i < numBuildingsPerSide_; ++i) {
			Building b;

			// ビルのサイズ (幅・奥行きを大きめにして巨大なメガストラクチャー感を演出)
			float w = 6.0f + rand01(rng) * 10.0f; // 幅を6.0m〜16.0mに拡大（従来の約2.5〜3倍）
			float d = 6.0f + rand01(rng) * 10.0f; // 奥行きも6.0m〜16.0mに拡大
			float h = 8.0f + std::pow(rand01(rng), 1.8f) * 32.0f; // べき乗で低層多め・時々超高層 (8m〜40m)

			// X位置: 道路の外側に多層的に配置
			float layer = rand01(rng);
			float x = side * (roadHalfWidth_ + w * 0.5f + layer * 35.0f);

			// Z位置
			float z = minZ_ + ((float)i / (float)numBuildingsPerSide_) * (maxZ_ - minZ_) + (randDist(rng) * 2.5f);

			// Y位置 (底面が地面 Y=baseY_ に接するよう高さの半分を加算)
			float y = baseY_ + h * 0.5f;

			b.basePosition = {x, y, z};
			b.scale = {w, h, d};

			// 配色の決定: 高層ビル(h>16m)または約25%の確率でアクセント(マゼンタ)、残りはシアンで統一
			float accentRoll = rand01(rng);
			if (h > 16.0f || accentRoll > 0.75f) {
				// アクセント（マゼンタ）
				b.baseColor = kColorMagenta;
			} else {
				// 通常の街並み（約75%）はシアン基調で統一
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

			buildings_.push_back(std::move(b));
		}
	}
}

void CityBackground::Update() {
	time_ += 0.016f;

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
