#include "Goal.h"
#include "Camera.h"
#include "TimeManager.h"
#include "WireframeObject.h"
#include "Vector2.h"
#ifdef USE_IMGUI
#include <imgui.h>
#endif
#include <cmath>
#include <numbers>

namespace {
	constexpr float kPi = std::numbers::pi_v<float>;
	constexpr float kTwoPi = 2.0f * kPi;
}

Goal::Goal() = default;
Goal::~Goal() = default;

void Goal::Initialize(const Vector3& pos, Camera* camera) {
	camera_ = camera;
	basePosition_ = pos;

	// トランスフォームの初期化
	worldTransform_.Initialize();
	worldTransform_.translation = pos;
	worldTransform_.scale = { 1.0f, 1.0f, 1.0f };
	worldTransform_.rotation = { 0.0f, 0.0f, 0.0f };
	worldTransform_.UpdateMatrix();

	// コライダーの初期設定（ゲート全体を包括する広大なAABB：幅18m、高さ12m、奥行き8m）
	// 自機がゲート開口部のどの位置を飛行していても確実に検知する
	SetShape(ColliderShape::kAABB);
	SetAABB({ { 18.0f, 12.0f, 8.0f } });

	// 1. 多重ワープトンネル（奥行き連続リング ＋ 縦梁レール ＋ トラス）
	BuildTunnelMesh();

	// 2. メインゲート外枠（左右パイロントラス ＋ 上部ガントリーブリッジ）
	BuildMainGateMesh();

	// 3. 対向回転アパーチャリング
	BuildApertureMesh();

	// 4. 頭上立体「GOAL」ネオンブロックサイン
	BuildGoalSignMesh();

	// 5. 進入誘導シェブロン（>>> 矢印）
	BuildChevronMesh();

	// 背景都市と同様の距離減衰フォグを設定
	SetFog(fogNear_, fogFar_);

	timer_ = 0.0f;
	rotAngleOuter_ = 0.0f;
	rotAngleInner_ = 0.0f;

	SetPosition(pos);
}

void Goal::SetPosition(const Vector3& pos) {
	basePosition_ = pos;
	worldTransform_.translation = pos;
	worldTransform_.UpdateMatrix();

	if (tunnelObject_) tunnelObject_->SetPosition(pos);
	if (gateFrameObject_) gateFrameObject_->SetPosition(pos);
	if (outerApertureObject_) outerApertureObject_->SetPosition(pos);
	if (innerApertureObject_) innerApertureObject_->SetPosition(pos);
	if (chevronObject_) chevronObject_->SetPosition(pos);

	for (size_t i = 0; i < goalLetterObjects_.size(); ++i) {
		if (goalLetterObjects_[i] && i < letterXOffsets_.size()) {
			Vector3 letterPos = pos;
			letterPos.x += letterXOffsets_[i];
			letterPos.y += 7.3f;
			goalLetterObjects_[i]->SetPosition(letterPos);
		}
	}
}

// ----------------------------------------------------
// 1. 多重ワープトンネルの構築
// ----------------------------------------------------
void Goal::BuildTunnelMesh() {
	tunnelObject_ = std::make_unique<WireframeObject>();
	tunnelObject_->Initialize();
	tunnelObject_->SetCamera(camera_);
	tunnelObject_->SetBlendMode(BlendMode::kAdd);

	// 5つの奥行き八角形リング（手前からゲート奥へ）
	const float zOffsets[] = { -20.0f, -14.0f, -8.0f, -2.0f, +4.0f };
	const float radii[]    = {   9.2f,   8.6f,  8.0f,  7.4f,  6.8f };
	const int kNumRings = 5;
	const int kNumCorners = 8;
	const float xFactor = 1.15f; // 横長のフライト領域に合わせた比率
	const float yFactor = 0.85f;

	std::vector<Vector4> vertices;
	std::vector<uint32_t> indices;

	for (int k = 0; k < kNumRings; ++k) {
		float z = zOffsets[k];
		float r = radii[k];
		uint32_t ringBase = static_cast<uint32_t>(vertices.size());

		// 8頂点
		for (int i = 0; i < kNumCorners; ++i) {
			float angle = (static_cast<float>(i) * kTwoPi / static_cast<float>(kNumCorners)) + (kPi / static_cast<float>(kNumCorners));
			float x = std::cos(angle) * r * xFactor;
			float y = std::sin(angle) * r * yFactor;
			vertices.push_back({ x, y, z, 1.0f });

			// リング円周のエッジ
			indices.push_back(ringBase + i);
			indices.push_back(ringBase + ((i + 1) % kNumCorners));
		}

		// 直前のリングとの縦梁・トラス接続
		if (k > 0) {
			uint32_t prevBase = ringBase - kNumCorners;
			for (int i = 0; i < kNumCorners; ++i) {
				// 縦梁レール
				indices.push_back(prevBase + i);
				indices.push_back(ringBase + i);

				// トラス対角線（偶数・奇数で交互に張る）
				indices.push_back(prevBase + i);
				indices.push_back(ringBase + ((i + 1) % kNumCorners));
			}
		}
	}

	tunnelObject_->CreateCustomMesh(vertices, indices);
	tunnelObject_->SetColor({ 0.0f, 0.70f, 0.95f, 0.85f }); // ディープサイバーシアン
}

// ----------------------------------------------------
// 2. メインゲート外枠の構築（左右パイロン ＋ ガントリートラス）
// ----------------------------------------------------
void Goal::BuildMainGateMesh() {
	gateFrameObject_ = std::make_unique<WireframeObject>();
	gateFrameObject_->Initialize();
	gateFrameObject_->SetCamera(camera_);
	gateFrameObject_->SetBlendMode(BlendMode::kAdd);

	std::vector<Vector4> v;
	std::vector<uint32_t> idx;

	auto AddLine = [&](const Vector3& p1, const Vector3& p2) {
		uint32_t base = static_cast<uint32_t>(v.size());
		v.push_back({ p1.x, p1.y, p1.z, 1.0f });
		v.push_back({ p2.x, p2.y, p2.z, 1.0f });
		idx.push_back(base);
		idx.push_back(base + 1);
	};

	// 奥行き2層（前面 Z = -0.3f, 背面 Z = +0.3f）で立体トラスを形成
	const float depths[] = { -0.35f, 0.35f };

	for (float z : depths) {
		// --- 左パイロン (X: -8.8m 〜 -10.2m, Y: -5.2m 〜 +6.4m) ---
		AddLine({ -8.8f, -5.2f, z }, { -8.8f,  6.4f, z });
		AddLine({ -10.2f, -5.2f, z }, { -10.2f,  6.4f, z });
		AddLine({ -8.8f, -5.2f, z }, { -10.2f, -5.2f, z });
		AddLine({ -8.8f,  6.4f, z }, { -10.2f,  6.4f, z });
		// パイロン内のXトラスラティス
		for (float y = -4.5f; y < 6.0f; y += 1.8f) {
			AddLine({ -8.8f, y, z }, { -10.2f, y + 1.8f, z });
			AddLine({ -10.2f, y, z }, { -8.8f, y + 1.8f, z });
			AddLine({ -8.8f, y, z }, { -10.2f, y, z });
		}

		// --- 右パイロン (X: +8.8m 〜 +10.2m, Y: -5.2m 〜 +6.4m) ---
		AddLine({  8.8f, -5.2f, z }, {  8.8f,  6.4f, z });
		AddLine({ 10.2f, -5.2f, z }, { 10.2f,  6.4f, z });
		AddLine({  8.8f, -5.2f, z }, { 10.2f, -5.2f, z });
		AddLine({  8.8f,  6.4f, z }, { 10.2f,  6.4f, z });
		// パイロン内のXトラスラティス
		for (float y = -4.5f; y < 6.0f; y += 1.8f) {
			AddLine({ 8.8f, y, z }, { 10.2f, y + 1.8f, z });
			AddLine({ 10.2f, y, z }, { 8.8f, y + 1.8f, z });
			AddLine({ 8.8f, y, z }, { 10.2f, y, z });
		}

		// --- 上部ガントリーブリッジ (Y: 5.2m & 6.4m, X: -8.8m 〜 +8.8m) ---
		AddLine({ -8.8f, 5.2f, z }, { 8.8f, 5.2f, z });
		AddLine({ -8.8f, 6.4f, z }, { 8.8f, 6.4f, z });
		// ガントリー内のジグザグトラス (/\/\/\)
		const int gantryDivs = 10;
		const float gantryStep = (8.8f * 2.0f) / static_cast<float>(gantryDivs);
		for (int i = 0; i < gantryDivs; ++i) {
			float x1 = -8.8f + static_cast<float>(i) * gantryStep;
			float x2 = x1 + gantryStep;
			float midX = (x1 + x2) * 0.5f;
			AddLine({ x1, 5.2f, z }, { midX, 6.4f, z });
			AddLine({ midX, 6.4f, z }, { x2, 5.2f, z });
			AddLine({ x1, 5.2f, z }, { x1, 6.4f, z });
		}

		// 上部コーナー面取り補強ブラケット
		AddLine({ -8.8f, 3.8f, z }, { -6.8f, 5.2f, z });
		AddLine({  8.8f, 3.8f, z }, {  6.8f, 5.2f, z });

		// サイン台座マウントレール (X: -3.8m 〜 +3.8m, Y: 6.4m 〜 7.0m)
		AddLine({ -3.8f, 6.4f, z }, { -3.8f, 7.1f, z });
		AddLine({  3.8f, 6.4f, z }, {  3.8f, 7.1f, z });
		AddLine({ -3.8f, 7.1f, z }, {  3.8f, 7.1f, z });
		AddLine({ -1.9f, 6.4f, z }, { -1.9f, 7.1f, z });
		AddLine({  1.9f, 6.4f, z }, {  1.9f, 7.1f, z });
	}

	// 前面と背面のトラス結合梁（Z方向の奥行き連結）
	const Vector2 connectPoints[] = {
		{ -8.8f,  6.4f }, { -10.2f,  6.4f }, { -8.8f, -5.2f }, { -10.2f, -5.2f },
		{  8.8f,  6.4f }, {  10.2f,  6.4f }, {  8.8f, -5.2f }, {  10.2f, -5.2f },
		{ -4.0f,  6.4f }, {  0.0f,  6.4f }, {  4.0f,  6.4f },
		{ -4.0f,  5.2f }, {  0.0f,  5.2f }, {  4.0f,  5.2f },
		{ -3.8f,  7.1f }, {  3.8f,  7.1f }
	};
	for (const auto& pt : connectPoints) {
		AddLine({ pt.x, pt.y, depths[0] }, { pt.x, pt.y, depths[1] });
	}

	gateFrameObject_->CreateCustomMesh(v, idx);
	gateFrameObject_->SetColor({ 0.2f, 0.90f, 1.0f, 1.0f }); // エレクトリックシアン
}

// ----------------------------------------------------
// 3. 対向回転アパーチャリングの構築
// ----------------------------------------------------
void Goal::BuildApertureMesh() {
	// === 外周リング（時計回り回転・8セグメント円弧 ＋ カリパー） ===
	outerApertureObject_ = std::make_unique<WireframeObject>();
	outerApertureObject_->Initialize();
	outerApertureObject_->SetCamera(camera_);
	outerApertureObject_->SetBlendMode(BlendMode::kAdd);

	{
		std::vector<Vector4> v;
		std::vector<uint32_t> idx;
		const int segments = 8;
		const int subdivPerSeg = 5;
		const float segAngle = kTwoPi / static_cast<float>(segments);
		const float arcAngle = segAngle * 0.72f;
		const float r = 6.2f;
		const float xf = 1.15f;
		const float yf = 0.85f;

		for (int s = 0; s < segments; ++s) {
			float startA = static_cast<float>(s) * segAngle;
			uint32_t base = static_cast<uint32_t>(v.size());

			for (int i = 0; i <= subdivPerSeg; ++i) {
				float a = startA + arcAngle * (static_cast<float>(i) / static_cast<float>(subdivPerSeg));
				float x = std::cos(a) * r * xf;
				float y = std::sin(a) * r * yf;
				v.push_back({ x, y, 0.0f, 1.0f });

				if (i > 0) {
					idx.push_back(base + i - 1);
					idx.push_back(base + i);
				}
			}

			// セグメント先頭に外向きのサイバーカリパー爪を追加
			float midA = startA + arcAngle * 0.5f;
			float tipX = std::cos(midA) * (r + 0.65f) * xf;
			float tipY = std::sin(midA) * (r + 0.65f) * yf;
			float inX  = std::cos(midA) * r * xf;
			float inY  = std::sin(midA) * r * yf;

			uint32_t cBase = static_cast<uint32_t>(v.size());
			v.push_back({ inX, inY, 0.0f, 1.0f });
			v.push_back({ tipX, tipY, 0.0f, 1.0f });
			idx.push_back(cBase);
			idx.push_back(cBase + 1);
		}

		outerApertureObject_->CreateCustomMesh(v, idx);
		outerApertureObject_->SetColor({ 0.0f, 1.0f, 0.65f, 0.90f }); // サイバーミントグリーン
	}

	// === 内周リング（反時計回り回転・ヘキサゴン ＋ 4隅照準ブラケット） ===
	innerApertureObject_ = std::make_unique<WireframeObject>();
	innerApertureObject_->Initialize();
	innerApertureObject_->SetCamera(camera_);
	innerApertureObject_->SetBlendMode(BlendMode::kAdd);

	{
		std::vector<Vector4> v;
		std::vector<uint32_t> idx;
		const int sides = 6;
		const float r = 4.8f;
		const float xf = 1.15f;
		const float yf = 0.85f;
		uint32_t hexBase = static_cast<uint32_t>(v.size());

		for (int i = 0; i < sides; ++i) {
			float a = static_cast<float>(i) * (kTwoPi / static_cast<float>(sides));
			v.push_back({ std::cos(a) * r * xf, std::sin(a) * r * yf, 0.0f, 1.0f });
			idx.push_back(hexBase + i);
			idx.push_back(hexBase + ((i + 1) % sides));
		}

		// 4隅のサイバーブラケット（中心に向かう [ ] 状の爪）
		const float bSize = 3.6f;
		const float bLen = 1.0f;
		const Vector2 bracketCorners[4] = {
			{ -bSize * xf,  bSize * yf },
			{  bSize * xf,  bSize * yf },
			{  bSize * xf, -bSize * yf },
			{ -bSize * xf, -bSize * yf }
		};
		const Vector2 bracketDirs[4] = {
			{  1.0f, -1.0f },
			{ -1.0f, -1.0f },
			{ -1.0f,  1.0f },
			{  1.0f,  1.0f }
		};

		for (int c = 0; c < 4; ++c) {
			Vector2 pt = bracketCorners[c];
			Vector2 dir = bracketDirs[c];
			uint32_t bBase = static_cast<uint32_t>(v.size());

			v.push_back({ pt.x, pt.y, 0.0f, 1.0f });
			v.push_back({ pt.x + dir.x * bLen, pt.y, 0.0f, 1.0f });
			v.push_back({ pt.x, pt.y + dir.y * bLen, 0.0f, 1.0f });

			idx.push_back(bBase);
			idx.push_back(bBase + 1);
			idx.push_back(bBase);
			idx.push_back(bBase + 2);
		}

		innerApertureObject_->CreateCustomMesh(v, idx);
		innerApertureObject_->SetColor({ 1.0f, 0.85f, 0.20f, 0.90f }); // ヴィヴィッドゴールド
	}
}

// ----------------------------------------------------
// 4. 頭上立体「GOAL」ネオンブロックサインの構築
// ----------------------------------------------------
void Goal::BuildGoalSignMesh() {
	goalLetterObjects_.clear();
	letterXOffsets_.clear();

	const std::string text = "GOAL";
	const float h = 1.55f;
	const float t = 0.24f;
	const float spacing = 0.32f;
	const float depth = 0.32f;
	const float halfDepth = depth * 0.5f;

	std::vector<LetterData> letters(text.size());
	float totalWidth = 0.0f;

	for (size_t i = 0; i < text.size(); ++i) {
		float w = 1.30f;
		if (text[i] == 'L') w = 1.10f;
		GenerateCharMesh(text[i], w, h, t, letters[i]);
		totalWidth += letters[i].width;
		if (i + 1 < text.size()) totalWidth += spacing;
	}

	float currentX = -totalWidth * 0.5f;

	for (size_t i = 0; i < letters.size(); ++i) {
		auto& l = letters[i];
		float xOffset = currentX + l.width * 0.5f;
		letterXOffsets_.push_back(xOffset);
		currentX += l.width + spacing;

		if (l.localVertices.empty() || l.baseEdges.empty()) {
			continue;
		}

		const size_t N = l.localVertices.size();
		std::vector<Vector4> v;
		std::vector<uint32_t> idx;
		v.reserve(N * 2);

		// 前面頂点 (Z = -halfDepth)
		for (const auto& pt : l.localVertices) {
			v.push_back({ pt.x - l.width * 0.5f, pt.y, -halfDepth, 1.0f });
		}
		// 背面頂点 (Z = +halfDepth)
		for (const auto& pt : l.localVertices) {
			v.push_back({ pt.x - l.width * 0.5f, pt.y, halfDepth, 1.0f });
		}

		// 前面エッジ
		for (const auto& edge : l.baseEdges) {
			idx.push_back(edge.first);
			idx.push_back(edge.second);
		}
		// 背面エッジ
		for (const auto& edge : l.baseEdges) {
			idx.push_back(static_cast<uint32_t>(edge.first + N));
			idx.push_back(static_cast<uint32_t>(edge.second + N));
		}
		// 奥行き結合梁（前面と背面を立体的に結ぶ）
		for (size_t k = 0; k < N; ++k) {
			idx.push_back(static_cast<uint32_t>(k));
			idx.push_back(static_cast<uint32_t>(k + N));
		}

		auto letterObj = std::make_unique<WireframeObject>();
		letterObj->Initialize();
		letterObj->SetCamera(camera_);
		letterObj->SetBlendMode(BlendMode::kAdd);
		letterObj->CreateCustomMesh(v, idx);
		letterObj->SetColor({ 0.30f, 1.0f, 0.20f, 1.0f }); // 眩いエレクトリックライム

		goalLetterObjects_.push_back(std::move(letterObj));
	}
}

// ----------------------------------------------------
// 5. 進入誘導シェブロン（>>> 矢印）の構築
// ----------------------------------------------------
void Goal::BuildChevronMesh() {
	chevronObject_ = std::make_unique<WireframeObject>();
	chevronObject_->Initialize();
	chevronObject_->SetCamera(camera_);
	chevronObject_->SetBlendMode(BlendMode::kAdd);

	std::vector<Vector4> v;
	std::vector<uint32_t> idx;

	auto AddLine = [&](const Vector3& p1, const Vector3& p2) {
		uint32_t base = static_cast<uint32_t>(v.size());
		v.push_back({ p1.x, p1.y, p1.z, 1.0f });
		v.push_back({ p2.x, p2.y, p2.z, 1.0f });
		idx.push_back(base);
		idx.push_back(base + 1);
	};

	// トンネル床面を奥へ指し示す3連の巨大シェブロン矢印 (>>>)
	const float chevronZ[] = { -16.0f, -10.0f, -4.0f };
	const float floorY = -4.8f;
	const float tipZOffset = 2.4f;
	const float wingX = 3.2f;

	for (float z : chevronZ) {
		Vector3 tip   = {  0.0f, floorY, z + tipZOffset };
		Vector3 left  = { -wingX, floorY, z - 0.8f };
		Vector3 right = {  wingX, floorY, z - 0.8f };

		// 外側メインライン
		AddLine(tip, left);
		AddLine(tip, right);

		// 内側平行ライン（二重ラインでネオン感を強調）
		Vector3 tipIn   = {  0.0f, floorY, z + tipZOffset - 0.5f };
		Vector3 leftIn  = { -wingX + 0.6f, floorY, z - 0.8f };
		Vector3 rightIn = {  wingX - 0.6f, floorY, z - 0.8f };
		AddLine(tipIn, leftIn);
		AddLine(tipIn, rightIn);
	}

	chevronObject_->CreateCustomMesh(v, idx);
	chevronObject_->SetColor({ 1.0f, 0.65f, 0.10f, 0.85f }); // 警戒色サイバーオレンジ
}

// ----------------------------------------------------
// ブロックフォント形状生成（G, O, A, L）
// ----------------------------------------------------
void Goal::GenerateCharMesh(char c, float w, float h, float t, LetterData& l) {
	l.width = w;
	l.localVertices.clear();
	l.baseEdges.clear();

	const float cut = t * 1.15f;

	switch (c) {
	case 'G': {
		const float midY = h * 0.45f;
		l.localVertices = {
			{w, h, 0.0f}, {cut, h, 0.0f}, {0.0f, h - cut, 0.0f},
			{0.0f, cut, 0.0f}, {cut, 0.0f, 0.0f}, {w, 0.0f, 0.0f},
			{w, midY, 0.0f}, {w * 0.52f, midY, 0.0f},
			{w * 0.52f, midY - t, 0.0f}, {w - t, midY - t, 0.0f},
			{w - t, t, 0.0f}, {cut + t * 0.2f, t, 0.0f},
			{t, cut + t * 0.2f, 0.0f}, {t, h - cut - t * 0.2f, 0.0f},
			{cut + t * 0.2f, h - t, 0.0f}, {w, h - t, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7},
			{7, 8}, {8, 9}, {9, 10}, {10, 11}, {11, 12}, {12, 13}, {13, 14}, {14, 15}, {15, 0}
		};
		break;
	}
	case 'O': {
		l.localVertices = {
			// 外周
			{cut, h, 0.0f}, {w - cut, h, 0.0f}, {w, h - cut, 0.0f}, {w, cut, 0.0f},
			{w - cut, 0.0f, 0.0f}, {cut, 0.0f, 0.0f}, {0.0f, cut, 0.0f}, {0.0f, h - cut, 0.0f},
			// 内窓
			{cut + t * 0.2f, h - t, 0.0f}, {w - cut - t * 0.2f, h - t, 0.0f},
			{w - t, h - cut - t * 0.2f, 0.0f}, {w - t, cut + t * 0.2f, 0.0f},
			{w - cut - t * 0.2f, t, 0.0f}, {cut + t * 0.2f, t, 0.0f},
			{t, cut + t * 0.2f, 0.0f}, {t, h - cut - t * 0.2f, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}, {7, 0},
			{8, 9}, {9, 10}, {10, 11}, {11, 12}, {12, 13}, {13, 14}, {14, 15}, {15, 8}
		};
		break;
	}
	case 'A': {
		l.localVertices = {
			{0.0f, 0.0f, 0.0f}, {w * 0.36f, h, 0.0f}, {w * 0.64f, h, 0.0f}, {w, 0.0f, 0.0f},
			{w - t, 0.0f, 0.0f}, {w * 0.68f, h * 0.42f, 0.0f}, {w * 0.32f, h * 0.42f, 0.0f}, {t, 0.0f, 0.0f},
			// 中窓
			{w * 0.38f, h * 0.60f, 0.0f}, {w * 0.46f, h - t, 0.0f},
			{w * 0.54f, h - t, 0.0f}, {w * 0.62f, h * 0.60f, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}, {7, 0},
			{8, 9}, {9, 10}, {10, 11}, {11, 8}
		};
		break;
	}
	case 'L': {
		l.localVertices = {
			{0.0f, 0.0f, 0.0f}, {0.0f, h, 0.0f}, {t, h, 0.0f},
			{t, t, 0.0f}, {w, t, 0.0f}, {w, 0.0f, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 0}
		};
		break;
	}
	default:
		break;
	}
}

// ----------------------------------------------------
// 毎フレーム更新
// ----------------------------------------------------
void Goal::Update() {
	float dt = TimeManager::GetInstance()->GetDeltaTime();
	timer_ += dt;

	// アパーチャの対向回転アニメーション
	rotAngleOuter_ += dt * 0.45f;
	rotAngleInner_ -= dt * 0.70f;

	// ネオンの呼吸パルス波（0.0 〜 1.0）
	float pulse = (std::sin(timer_ * 3.6f) + 1.0f) * 0.5f;

	// 各パーツのカラー更新（加算ブレンドによる眩いネオングロー）
	if (tunnelObject_) {
		tunnelObject_->SetPosition(basePosition_);
		tunnelObject_->SetColor({ 0.0f, 0.70f + 0.25f * pulse, 0.95f, 0.85f });
		tunnelObject_->Update();
	}

	if (gateFrameObject_) {
		gateFrameObject_->SetPosition(basePosition_);
		gateFrameObject_->SetColor({ 0.20f, 0.88f + 0.12f * pulse, 1.0f, 1.0f });
		gateFrameObject_->Update();
	}

	if (outerApertureObject_) {
		outerApertureObject_->SetPosition(basePosition_);
		outerApertureObject_->SetRotation({ 0.0f, 0.0f, rotAngleOuter_ });
		outerApertureObject_->SetColor({ 0.0f, 1.0f, 0.60f + 0.35f * pulse, 0.90f });
		outerApertureObject_->Update();
	}

	if (innerApertureObject_) {
		innerApertureObject_->SetPosition(basePosition_);
		innerApertureObject_->SetRotation({ 0.0f, 0.0f, rotAngleInner_ });
		innerApertureObject_->SetColor({ 1.0f, 0.80f + 0.20f * pulse, 0.20f, 0.90f });
		innerApertureObject_->Update();
	}

	if (chevronObject_) {
		chevronObject_->SetPosition(basePosition_);
		chevronObject_->SetColor({ 1.0f, 0.65f + 0.30f * pulse, 0.10f, 0.85f });
		chevronObject_->Update();
	}

	// 「GOAL」サインの微細な浮遊バウンス
	float signBounce = std::sin(timer_ * 2.5f) * 0.14f;
	Vector4 signColor = { 0.25f + 0.35f * pulse, 1.0f, 0.15f + 0.25f * (1.0f - pulse), 1.0f };

	for (size_t i = 0; i < goalLetterObjects_.size(); ++i) {
		if (goalLetterObjects_[i] && i < letterXOffsets_.size()) {
			Vector3 letterPos = basePosition_;
			letterPos.x += letterXOffsets_[i];
			letterPos.y += 7.2f + signBounce;
			goalLetterObjects_[i]->SetPosition(letterPos);
			goalLetterObjects_[i]->SetColor(signColor);
			goalLetterObjects_[i]->Update();
		}
	}
}

// ----------------------------------------------------
// 描画
// ----------------------------------------------------
void Goal::Draw() {
	if (tunnelObject_) tunnelObject_->Draw();
	if (gateFrameObject_) gateFrameObject_->Draw();
	if (outerApertureObject_) outerApertureObject_->Draw();
	if (innerApertureObject_) innerApertureObject_->Draw();
	if (chevronObject_) chevronObject_->Draw();

	for (auto& letter : goalLetterObjects_) {
		if (letter) letter->Draw();
	}
}

void Goal::SetFog(float fogNear, float fogFar) {
	fogNear_ = fogNear;
	fogFar_ = fogFar;

	if (tunnelObject_) tunnelObject_->SetFog(fogNear, fogFar);
	if (gateFrameObject_) gateFrameObject_->SetFog(fogNear, fogFar);
	if (outerApertureObject_) outerApertureObject_->SetFog(fogNear, fogFar);
	if (innerApertureObject_) innerApertureObject_->SetFog(fogNear, fogFar);
	if (chevronObject_) chevronObject_->SetFog(fogNear, fogFar);

	for (auto& letter : goalLetterObjects_) {
		if (letter) letter->SetFog(fogNear, fogFar);
	}
}

void Goal::OnCollision() {
}

Vector3 Goal::GetWorldPosition() {
	return basePosition_;
}

void Goal::DrawImGui(const char* windowName) {
#ifdef USE_IMGUI
	if (ImGui::Begin(windowName)) {
		ImGui::Text("Wireframe Cyber Goal Gate");
		ImGui::DragFloat3("Base Position", &basePosition_.x, 0.5f);
		if (ImGui::Button("Sync Position")) {
			SetPosition(basePosition_);
		}
		ImGui::Separator();
		float fNear = fogNear_;
		float fFar = fogFar_;
		bool fogChanged = false;
		if (ImGui::SliderFloat("Goal Fog Near", &fNear, 10.0f, 300.0f, "%.1f m")) fogChanged = true;
		if (ImGui::SliderFloat("Goal Fog Far", &fFar, 50.0f, 600.0f, "%.1f m")) fogChanged = true;
		if (fogChanged) {
			SetFog(fNear, fFar);
		}
		ImGui::Separator();
		ImGui::Text("Parts: Tunnel, Pylons, Gantry, Apertures, Sign, Chevrons");
	}
	ImGui::End();
#endif
}
