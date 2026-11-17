#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "Goal.h"
#include "Camera.h"
#include "TimeManager.h"
#include "WireframeObject.h"
#include "MathUtility.h"
#include "Vector2.h"
#ifdef USE_IMGUI
#include <imgui.h>
#endif
#include <cmath>
#include <numbers>
#include <algorithm>
#include <cstdlib>

namespace {
	constexpr float kPi = std::numbers::pi_v<float>;
	constexpr float kTwoPi = 2.0f * kPi;

	float RandRange(float minVal, float maxVal) {
		float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
		return minVal + (maxVal - minVal) * t;
	}

	float EaseOutCubic(float t) {
		t = (std::clamp)(t, 0.0f, 1.0f);
		float inv = 1.0f - t;
		return 1.0f - inv * inv * inv;
	}
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

	// コライダーの初期設定（横長ワイドなゲート面の光の壁に合わせた判定：幅34m、高さ15m、奥行き2.2m）
	// 自機がゲート壁面に激突した瞬間をジャスト検知する
	SetShape(ColliderShape::kAABB);
	SetAABB({ { 34.0f, 15.0f, 2.2f } });

	// 1. コースを塞ぐ巨大な光の壁（セキュリティゲート・ホログラムバリア）
	BuildSecurityBarrierMesh();

	// 2. コース中央を塞ぐ巨大ホログラム「GOAL」文字
	BuildCenterGoalHologramMesh();

	// 3. 破砕時ボクセル用インスタンシングメッシュの初期化
	InitializeVoxelInstancing();

	// 背景都市と同様の距離減衰フォグを設定
	SetFog(fogNear_, fogFar_);

	Reset();
	SetPosition(pos);
}

void Goal::Reset() {
	timer_ = 0.0f;

	isAppearing_ = false;
	appearProgress_ = 0.0f;

	isInHitStop_ = false;
	hitStopTimer_ = 0.0f;
	impactCrackObject_.reset();

	isShattered_ = false;
	shatterTimer_ = 0.0f;
	voxels_.clear();
}

void Goal::TriggerAppear() {
	if (!isAppearing_ && appearProgress_ < 1.0f) {
		isAppearing_ = true;
	}
}

void Goal::ForceAppearImmediate() {
	isAppearing_ = true;
	appearProgress_ = 1.0f;
}

void Goal::SetPosition(const Vector3& pos) {
	basePosition_ = pos;
	worldTransform_.translation = pos;
	worldTransform_.UpdateMatrix();

	if (barrierWallObject_) barrierWallObject_->SetPosition(pos);
	if (barrierCoreObject_) barrierCoreObject_->SetPosition(pos);
	if (scanlineBeamObject_) scanlineBeamObject_->SetPosition(pos);
	if (impactCrackObject_) impactCrackObject_->SetPosition(pos);

	for (size_t i = 0; i < centerGoalLetterObjects_.size(); ++i) {
		if (centerGoalLetterObjects_[i] && i < centerLetterXOffsets_.size()) {
			Vector3 letterPos = pos;
			letterPos.x += centerLetterXOffsets_[i];
			letterPos.y += -1.35f;
			centerGoalLetterObjects_[i]->SetPosition(letterPos);
		}
	}
}

// ----------------------------------------------------
// 1. 進行コースを塞ぐ巨大な光の壁（ホログラム・セキュリティゲート）の構築
// ----------------------------------------------------
void Goal::BuildSecurityBarrierMesh() {
	// === A. メイン・ホログラムレーザーグリッド＆ハニカム遮蔽壁 ===
	barrierWallObject_ = std::make_unique<WireframeObject>();
	barrierWallObject_->Initialize();
	barrierWallObject_->SetCamera(camera_);
	barrierWallObject_->SetBlendMode(BlendMode::kAdd);

	{
		std::vector<Vector4> v;
		std::vector<uint32_t> idx;

		auto AddLine = [&](const Vector3& p1, const Vector3& p2) {
			uint32_t base = static_cast<uint32_t>(v.size());
			v.push_back({ p1.x, p1.y, p1.z, 1.0f });
			v.push_back({ p2.x, p2.y, p2.z, 1.0f });
			idx.push_back(base);
			idx.push_back(base + 1);
		};

		const float halfW = 16.50f;
		const float halfH = 5.00f;

		// 1. 外周セキュリティシールド三重枠（面取りコーナー付きホログラムフレーム）
		AddLine({ -halfW, -halfH, 0.0f }, {  halfW, -halfH, 0.0f });
		AddLine({  halfW, -halfH, 0.0f }, {  halfW,  halfH, 0.0f });
		AddLine({  halfW,  halfH, 0.0f }, { -halfW,  halfH, 0.0f });
		AddLine({ -halfW,  halfH, 0.0f }, { -halfW, -halfH, 0.0f });

		AddLine({ -halfW + 0.35f, -halfH + 0.35f, 0.0f }, {  halfW - 0.35f, -halfH + 0.35f, 0.0f });
		AddLine({  halfW - 0.35f, -halfH + 0.35f, 0.0f }, {  halfW - 0.35f,  halfH - 0.35f, 0.0f });
		AddLine({  halfW - 0.35f,  halfH - 0.35f, 0.0f }, { -halfW + 0.35f,  halfH - 0.35f, 0.0f });
		AddLine({ -halfW + 0.35f,  halfH - 0.35f, 0.0f }, { -halfW + 0.35f, -halfH + 0.35f, 0.0f });

		// 左右端のホログラムエミッターレール
		AddLine({ -halfW - 0.30f, -halfH + 0.5f, -0.08f }, { -halfW - 0.30f,  halfH - 0.5f, -0.08f });
		AddLine({  halfW + 0.30f, -halfH + 0.5f, -0.08f }, {  halfW + 0.30f,  halfH - 0.5f, -0.08f });

		// 2. 垂直・水平セキュリティレーザーグリッド（コース全面を隙間なく塞ぐ横長ワイドな光の格子）
		const int cols = 24;
		const int rows = 10;
		for (int c = 1; c < cols; ++c) {
			float x = -halfW + (halfW * 2.0f) * (static_cast<float>(c) / static_cast<float>(cols));
			// 中央のGOAL文字プレート領域(Y: -1.80 ~ +1.80, X: -8.6 ~ +8.6)は視認性のため上下で分割接続
			if (std::abs(x) < 8.60f) {
				AddLine({ x, -halfH, 0.0f }, { x, -1.82f, 0.0f });
				AddLine({ x,  1.82f, 0.0f }, { x,  halfH, 0.0f });
			} else {
				AddLine({ x, -halfH, 0.0f }, { x,  halfH, 0.0f });
			}
		}

		for (int r = 1; r < rows; ++r) {
			float y = -halfH + (halfH * 2.0f) * (static_cast<float>(r) / static_cast<float>(rows));
			if (std::abs(y) < 1.80f) {
				AddLine({ -halfW, y, 0.0f }, { -8.70f, y, 0.0f });
				AddLine({  8.70f, y, 0.0f }, {  halfW, y, 0.0f });
			} else {
				AddLine({ -halfW, y, 0.0f }, {  halfW, y, 0.0f });
			}
		}

		// 3. ダイヤ・ハニカム状のエネルギーバリア結晶格子（ガラス壁の質感）
		const float cellW = (halfW * 2.0f) / 12.0f;
		const float cellH = (halfH * 2.0f) / 5.0f;
		for (int r = 0; r < 5; ++r) {
			float y0 = -halfH + r * cellH;
			float y1 = y0 + cellH;
			float ym = (y0 + y1) * 0.5f;
			for (int c = 0; c < 12; ++c) {
				float x0 = -halfW + c * cellW;
				float x1 = x0 + cellW;
				float xm = (x0 + x1) * 0.5f;
				if (std::abs(ym) < 1.65f && std::abs(xm) < 8.4f) continue;

				AddLine({ xm, y0, 0.02f }, { x1, ym, 0.02f });
				AddLine({ x1, ym, 0.02f }, { xm, y1, 0.02f });
				AddLine({ xm, y1, 0.02f }, { x0, ym, 0.02f });
				AddLine({ x0, ym, 0.02f }, { xm, y0, 0.02f });
			}
		}

		// 4. 中央「GOAL」ホログラムプレートのセキュリティフレーム（上下の二重レーザーバー＆ブラケット）
		const float plateW = 8.70f;
		const float plateH = 1.84f;
		AddLine({ -plateW,  plateH, -0.05f }, {  plateW,  plateH, -0.05f });
		AddLine({ -plateW, -plateH, -0.05f }, {  plateW, -plateH, -0.05f });
		AddLine({ -plateW,  plateH + 0.14f, -0.05f }, {  plateW,  plateH + 0.14f, -0.05f });
		AddLine({ -plateW, -plateH - 0.14f, -0.05f }, {  plateW, -plateH - 0.14f, -0.05f });

		AddLine({ -plateW, -plateH, -0.05f }, { -plateW - 0.55f, -plateH + 0.45f, -0.05f });
		AddLine({ -plateW - 0.55f, -plateH + 0.45f, -0.05f }, { -plateW - 0.55f,  plateH - 0.45f, -0.05f });
		AddLine({ -plateW - 0.55f,  plateH - 0.45f, -0.05f }, { -plateW,  plateH, -0.05f });

		AddLine({  plateW, -plateH, -0.05f }, {  plateW + 0.55f, -plateH + 0.45f, -0.05f });
		AddLine({  plateW + 0.55f, -plateH + 0.45f, -0.05f }, {  plateW + 0.55f,  plateH - 0.45f, -0.05f });
		AddLine({  plateW + 0.55f,  plateH - 0.45f, -0.05f }, {  plateW,  plateH, -0.05f });

		barrierWallObject_->CreateCustomMesh(v, idx);
		barrierWallObject_->SetColor({ 0.05f, 0.92f, 1.0f, 0.85f });
	}

	// === B. 内層エネルギーフィールド（高密度ホログラム走査線＆四隅セキュリティ警告ストライプ） ===
	barrierCoreObject_ = std::make_unique<WireframeObject>();
	barrierCoreObject_->Initialize();
	barrierCoreObject_->SetCamera(camera_);
	barrierCoreObject_->SetBlendMode(BlendMode::kAdd);

	{
		std::vector<Vector4> v;
		std::vector<uint32_t> idx;

		auto AddLine = [&](const Vector3& p1, const Vector3& p2) {
			uint32_t base = static_cast<uint32_t>(v.size());
			v.push_back({ p1.x, p1.y, p1.z, 1.0f });
			v.push_back({ p2.x, p2.y, p2.z, 1.0f });
			idx.push_back(base);
			idx.push_back(base + 1);
		};

		const float halfW = 16.35f;
		const float halfH = 4.85f;

		// 38本の高密度ホログラム水平スキャンライン（光の壁の面感を強調）
		const int scanCount = 38;
		for (int i = 0; i <= scanCount; ++i) {
			float y = -halfH + (halfH * 2.0f) * (static_cast<float>(i) / static_cast<float>(scanCount));
			AddLine({ -halfW, y, 0.05f }, { halfW, y, 0.05f });
		}

		// 四隅のセキュリティ警告ストライプ
		for (int i = 1; i <= 6; ++i) {
			float offset = static_cast<float>(i) * 0.75f;
			AddLine({ -halfW + offset,  halfH, -0.03f }, { -halfW,  halfH - offset, -0.03f });
			AddLine({  halfW - offset,  halfH, -0.03f }, {  halfW,  halfH - offset, -0.03f });
			AddLine({ -halfW + offset, -halfH, -0.03f }, { -halfW, -halfH + offset, -0.03f });
			AddLine({  halfW - offset, -halfH, -0.03f }, {  halfW, -halfH + offset, -0.03f });
		}

		barrierCoreObject_->CreateCustomMesh(v, idx);
		barrierCoreObject_->SetColor({ 0.20f, 0.98f, 1.0f, 0.42f });
	}

	// === C. 上下スイープ走査レーザービーム ===
	scanlineBeamObject_ = std::make_unique<WireframeObject>();
	scanlineBeamObject_->Initialize();
	scanlineBeamObject_->SetCamera(camera_);
	scanlineBeamObject_->SetBlendMode(BlendMode::kAdd);

	{
		std::vector<Vector4> v;
		std::vector<uint32_t> idx;
		const float halfW = 16.50f;
		const float yOffsets[] = { -0.12f, -0.04f, 0.0f, 0.04f, 0.12f };
		for (float yo : yOffsets) {
			uint32_t base = static_cast<uint32_t>(v.size());
			v.push_back({ -halfW, yo, -0.08f, 1.0f });
			v.push_back({  halfW, yo, -0.08f, 1.0f });
			idx.push_back(base);
			idx.push_back(base + 1);
		}
		scanlineBeamObject_->CreateCustomMesh(v, idx);
		scanlineBeamObject_->SetColor({ 0.85f, 1.0f, 1.0f, 0.95f });
	}
}

// ----------------------------------------------------
// 2. コース中央を塞ぐ巨大ホログラム「GOAL」文字の構築
// ----------------------------------------------------
void Goal::BuildCenterGoalHologramMesh() {
	centerGoalLetterObjects_.clear();
	centerLetterXOffsets_.clear();
	centerLetterSamplePoints_.clear();

	const std::string text = "GOAL";
	const float h = 2.75f;
	const float t = 0.45f;
	const float spacing = 0.95f;
	const float depth = 0.48f;
	const float halfDepth = depth * 0.5f;

	std::vector<LetterData> letters(text.size());
	float totalWidth = 0.0f;

	for (size_t i = 0; i < text.size(); ++i) {
		float w = 2.95f;
		if (text[i] == 'L') w = 2.50f;
		GenerateCharMesh(text[i], w, h, t, letters[i]);
		totalWidth += letters[i].width;
		if (i + 1 < text.size()) totalWidth += spacing;
	}

	float currentX = -totalWidth * 0.5f;

	for (size_t i = 0; i < letters.size(); ++i) {
		auto& l = letters[i];
		float xOffset = currentX + l.width * 0.5f;
		centerLetterXOffsets_.push_back(xOffset);
		currentX += l.width + spacing;

		if (l.localVertices.empty() || l.baseEdges.empty()) {
			continue;
		}

		const size_t N = l.localVertices.size();
		std::vector<Vector4> v;
		std::vector<uint32_t> idx;
		v.reserve(N * 3);

		// 前面・中間・背面の3層ホログラムで立体感を出す
		for (const auto& pt : l.localVertices) {
			v.push_back({ pt.x - l.width * 0.5f, pt.y, -halfDepth, 1.0f });
		}
		for (const auto& pt : l.localVertices) {
			v.push_back({ pt.x - l.width * 0.5f, pt.y, 0.0f, 1.0f });
		}
		for (const auto& pt : l.localVertices) {
			v.push_back({ pt.x - l.width * 0.5f, pt.y, halfDepth, 1.0f });
		}

		for (const auto& edge : l.baseEdges) {
			idx.push_back(edge.first);
			idx.push_back(edge.second);

			idx.push_back(static_cast<uint32_t>(edge.first + N));
			idx.push_back(static_cast<uint32_t>(edge.second + N));

			idx.push_back(static_cast<uint32_t>(edge.first + N * 2));
			idx.push_back(static_cast<uint32_t>(edge.second + N * 2));

			// 破砕時に「GOAL」文字のライン上からボクセルが弾け飛ぶようサンプル座標を記録
			const Vector3& p0 = l.localVertices[edge.first];
			const Vector3& p1 = l.localVertices[edge.second];
			for (int s = 0; s < 3; ++s) {
				float ratio = static_cast<float>(s) / 3.0f;
				Vector3 sampleLocal = {
					(p0.x + (p1.x - p0.x) * ratio) - l.width * 0.5f + xOffset,
					(p0.y + (p1.y - p0.y) * ratio) - 1.35f,
					0.0f
				};
				centerLetterSamplePoints_.push_back(sampleLocal);
			}
		}

		for (size_t k = 0; k < N; ++k) {
			idx.push_back(static_cast<uint32_t>(k));
			idx.push_back(static_cast<uint32_t>(k + N * 2));
		}

		auto letterObj = std::make_unique<WireframeObject>();
		letterObj->Initialize();
		letterObj->SetCamera(camera_);
		letterObj->SetBlendMode(BlendMode::kAdd);
		letterObj->CreateCustomMesh(v, idx);
		letterObj->SetColor({ 0.25f, 1.0f, 0.45f, 1.0f });

		centerGoalLetterObjects_.push_back(std::move(letterObj));
	}
}

// ----------------------------------------------------
// 3. ヒットストップ時のガラス亀裂（インパクトクラック）の構築
// ----------------------------------------------------
void Goal::BuildImpactCrackMesh(const Vector3& impactWorldPos) {
	impactCrackObject_ = std::make_unique<WireframeObject>();
	impactCrackObject_->Initialize();
	impactCrackObject_->SetCamera(camera_);
	impactCrackObject_->SetBlendMode(BlendMode::kAdd);

	std::vector<Vector4> v;
	std::vector<uint32_t> idx;

	auto AddLine = [&](const Vector3& p1, const Vector3& p2) {
		uint32_t base = static_cast<uint32_t>(v.size());
		v.push_back({ p1.x, p1.y, p1.z, 1.0f });
		v.push_back({ p2.x, p2.y, p2.z, 1.0f });
		idx.push_back(base);
		idx.push_back(base + 1);
	};

	float cx = (std::clamp)(impactWorldPos.x - basePosition_.x, -12.0f, 12.0f);
	float cy = (std::clamp)(impactWorldPos.y - basePosition_.y, -3.8f, 3.8f);
	const float cz = -0.12f;

	const int numRays = 16;
	const int stepsPerRay = 6;

	std::vector<std::vector<Vector3>> rayPoints(numRays, std::vector<Vector3>(stepsPerRay + 1));
	for (int r = 0; r < numRays; ++r) {
		float baseAngle = (static_cast<float>(r) / static_cast<float>(numRays)) * kTwoPi;
		rayPoints[r][0] = { cx, cy, cz };
		for (int s = 1; s <= stepsPerRay; ++s) {
			float dist = (static_cast<float>(s) / static_cast<float>(stepsPerRay)) * 12.5f;
			float jitterAngle = baseAngle + std::sin(static_cast<float>(r * 7 + s * 13)) * 0.14f;
			float px = (std::clamp)(cx + std::cos(jitterAngle) * dist * 1.45f, -16.50f, 16.50f);
			float py = (std::clamp)(cy + std::sin(jitterAngle) * dist * 0.80f, -5.00f, 5.00f);
			rayPoints[r][s] = { px, py, cz };
		}
	}

	for (int s = 0; s < stepsPerRay; ++s) {
		for (int r = 0; r < numRays; ++r) {
			AddLine(rayPoints[r][s], rayPoints[r][s + 1]);

			if (s >= 1 && ((r + s) % 2 == 0)) {
				int nextR = (r + 1) % numRays;
				AddLine(rayPoints[r][s], rayPoints[nextR][s]);
			}

			if (s >= 2) {
				Vector3 dir = {
					rayPoints[r][s + 1].x - rayPoints[r][s].x,
					rayPoints[r][s + 1].y - rayPoints[r][s].y,
					0.0f
				};
				Vector3 branchEnd = {
					(std::clamp)(rayPoints[r][s].x + dir.x * 0.55f - dir.y * 0.42f, -16.50f, 16.50f),
					(std::clamp)(rayPoints[r][s].y + dir.y * 0.55f + dir.x * 0.42f, -5.00f, 5.00f),
					cz
				};
				AddLine(rayPoints[r][s], branchEnd);
			}
		}
	}

	impactCrackObject_->CreateCustomMesh(v, idx);
	impactCrackObject_->SetPosition(basePosition_);
	impactCrackObject_->SetFog(fogNear_, fogFar_);
	impactCrackObject_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
}

// ----------------------------------------------------
// 4. 破砕時ボクセル用インスタンシングの初期化
// ----------------------------------------------------
void Goal::InitializeVoxelInstancing() {
	voxelOuterObject_ = std::make_unique<WireframeObject>();
	voxelOuterObject_->Initialize();
	voxelOuterObject_->SetCamera(camera_);
	voxelOuterObject_->SetBlendMode(BlendMode::kAdd);
	voxelOuterObject_->CreateBox({ 1.0f, 1.0f, 1.0f });
	voxelOuterObject_->CreateInstanceBuffer(kMaxVoxels);

	voxelInnerObject_ = std::make_unique<WireframeObject>();
	voxelInnerObject_->Initialize();
	voxelInnerObject_->SetCamera(camera_);
	voxelInnerObject_->SetBlendMode(BlendMode::kAdd);
	voxelInnerObject_->CreateOctahedron(0.52f);
	voxelInnerObject_->CreateInstanceBuffer(kMaxVoxels);
}

// ----------------------------------------------------
// 自機激突瞬間のヒットストップ演出開始
// ----------------------------------------------------
void Goal::StartImpactHitStop(const Vector3& impactWorldPos) {
	if (isInHitStop_ || isShattered_) return;

	ForceAppearImmediate();
	isInHitStop_ = true;
	hitStopTimer_ = 0.0f;
	impactPos_ = impactWorldPos;

	BuildImpactCrackMesh(impactWorldPos);
	if (impactCrackObject_) {
		impactCrackObject_->SetDrawRatio(0.25f);
		impactCrackObject_->Update();
	}
}

// ----------------------------------------------------
// ゲート破砕・無数のサイバー発光ボクセル飛散開始
// ----------------------------------------------------
void Goal::Shatter(const Vector3& impactWorldPos, const Vector3& forwardDir) {
	if (isShattered_) return;

	isShattered_ = true;
	isInHitStop_ = false;
	shatterTimer_ = 0.0f;
	impactPos_ = impactWorldPos;
	voxels_.clear();
	voxels_.reserve(kMaxVoxels);

	Vector3 fwd = forwardDir;
	float fwdLen = std::sqrt(fwd.x * fwd.x + fwd.y * fwd.y + fwd.z * fwd.z);
	if (fwdLen > 0.0001f) {
		fwd.x /= fwdLen;
		fwd.y /= fwdLen;
		fwd.z /= fwdLen;
	} else {
		fwd = { 0.0f, 0.0f, 1.0f };
	}

	auto AddVoxel = [&](const Vector3& worldPt, const Vector4& col, float scaleBase, float speedMultiplier) {
		if (voxels_.size() >= kMaxVoxels) return;

		VoxelPiece vp;
		vp.position = worldPt;

		float dx = worldPt.x - impactWorldPos.x;
		float dy = worldPt.y - impactWorldPos.y;
		float dist = std::sqrt(dx * dx + dy * dy);
		float invDist = (dist > 0.05f) ? (1.0f / dist) : 1.0f;
		float rx = (dist > 0.05f) ? (dx * invDist) : RandRange(-1.0f, 1.0f);
		float ry = (dist > 0.05f) ? (dy * invDist) : RandRange(-1.0f, 1.0f);

		float centerFactor = (std::max)(0.2f, 1.0f - (dist / 10.5f));
		float forwardSpeed = (16.0f + centerFactor * 22.0f + RandRange(0.0f, 12.0f)) * speedMultiplier;
		float radialSpeed  = (3.5f + (1.0f - centerFactor * 0.4f) * 14.0f + RandRange(0.0f, 6.0f)) * speedMultiplier;

		vp.velocity = {
			fwd.x * forwardSpeed + rx * radialSpeed + RandRange(-2.5f, 2.5f),
			fwd.y * forwardSpeed + ry * radialSpeed + RandRange(-2.0f, 3.0f),
			fwd.z * forwardSpeed + RandRange(-2.0f, 6.0f)
		};

		vp.rotation = {
			RandRange(0.0f, kTwoPi),
			RandRange(0.0f, kTwoPi),
			RandRange(0.0f, kTwoPi)
		};
		vp.angularVelocity = {
			RandRange(-11.0f, 11.0f),
			RandRange(-11.0f, 11.0f),
			RandRange(-11.0f, 11.0f)
		};

		float s = scaleBase * RandRange(0.75f, 1.30f);
		if ((voxels_.size() % 5) == 0) {
			vp.baseScale = { s * 1.45f, s * 0.75f, s * 0.75f };
		} else {
			vp.baseScale = { s, s, s };
		}

		vp.color = col;
		vp.lifeTime = RandRange(1.45f, 2.35f);
		vp.elapsed = 0.0f;

		voxels_.push_back(vp);
	};

	// 1. コース中央の「GOAL」ホログラム文字がそのまま発光ボクセルになって弾け飛ぶ！
	for (size_t i = 0; i < centerLetterSamplePoints_.size(); ++i) {
		const Vector3& localPt = centerLetterSamplePoints_[i];
		Vector3 worldPt = {
			basePosition_.x + localPt.x + RandRange(-0.08f, 0.08f),
			basePosition_.y + localPt.y + RandRange(-0.08f, 0.08f),
			basePosition_.z + RandRange(-0.20f, 0.20f)
		};

		Vector4 col;
		if (i % 3 == 0)      col = { 0.35f, 1.00f, 0.30f, 1.0f }; // エレクトリックライム
		else if (i % 3 == 1) col = { 1.00f, 0.92f, 0.25f, 1.0f }; // ソーラーゴールド
		else                 col = { 0.85f, 1.00f, 1.00f, 1.0f }; // ブリリアントホワイト

		AddVoxel(worldPt, col, 0.34f, 1.15f);
	}

	// 2. 自機激突ポイント中心の高密度クリスタル・コアボクセル群
	for (int i = 0; i < 65; ++i) {
		float angle = RandRange(0.0f, kTwoPi);
		float radius = RandRange(0.0f, 2.6f);
		Vector3 worldPt = {
			impactWorldPos.x + std::cos(angle) * radius,
			impactWorldPos.y + std::sin(angle) * radius,
			basePosition_.z + RandRange(-0.3f, 0.3f)
		};

		Vector4 col;
		int cType = i % 4;
		if (cType == 0)      col = { 1.00f, 1.00f, 1.00f, 1.0f }; // ホワイト
		else if (cType == 1) col = { 0.10f, 0.98f, 1.00f, 1.0f }; // ネオンシアン
		else if (cType == 2) col = { 1.00f, 0.20f, 0.88f, 1.0f }; // サイバーマゼンタ
		else                 col = { 0.20f, 1.00f, 0.65f, 1.0f }; // サイバーミント

		AddVoxel(worldPt, col, 0.30f, 1.25f);
	}

	// 3. 光の壁（横長ワイドなセキュリティゲート全面 X: -16.0〜+16.0, Y: -4.8〜+4.8）のガラス破砕ボクセル群
	const int gridCols = 19;
	const int gridRows = 10;
	for (int r = 0; r < gridRows; ++r) {
		float ny = (static_cast<float>(r) / static_cast<float>(gridRows - 1)) * 2.0f - 1.0f;
		for (int c = 0; c < gridCols; ++c) {
			if (voxels_.size() >= kMaxVoxels) break;
			float nx = (static_cast<float>(c) / static_cast<float>(gridCols - 1)) * 2.0f - 1.0f;

			Vector3 worldPt = {
				basePosition_.x + nx * 15.8f + RandRange(-0.35f, 0.35f),
				basePosition_.y + ny * 4.7f + RandRange(-0.22f, 0.22f),
				basePosition_.z + RandRange(-0.25f, 0.25f)
			};

			Vector4 col;
			int hash = (r * 7 + c * 13) % 5;
			if (hash == 0 || hash == 1) col = { 0.05f, 0.92f, 1.00f, 1.0f }; // エレクトリックシアン
			else if (hash == 2)         col = { 0.00f, 1.00f, 0.68f, 1.0f }; // サイバーミント
			else if (hash == 3)         col = { 1.00f, 0.22f, 0.85f, 1.0f }; // ネオンマゼンタ
			else                        col = { 1.00f, 0.88f, 0.25f, 1.0f }; // ヴィヴィッドゴールド

			AddVoxel(worldPt, col, 0.36f, 1.0f);
		}
	}

	// 初回フレーム用に即座にインスタンスバッファを更新
	Update(0.0f);
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
			{cut, h, 0.0f}, {w - cut, h, 0.0f}, {w, h - cut, 0.0f}, {w, cut, 0.0f},
			{w - cut, 0.0f, 0.0f}, {cut, 0.0f, 0.0f}, {0.0f, cut, 0.0f}, {0.0f, h - cut, 0.0f},
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
void Goal::Update(float timeScale) {
	float rawDt = TimeManager::GetInstance()->GetDeltaTime();
	float dt = rawDt * timeScale;
	timer_ += rawDt;

	// 1. ゴール接近時のホログラムゲート出現アニメーション更新
	if (isAppearing_ && appearProgress_ < 1.0f) {
		appearProgress_ = (std::min)(1.0f, appearProgress_ + rawDt / kAppearDuration);
	}

	float barrierProg = (std::clamp)(appearProgress_ / 0.75f, 0.0f, 1.0f);
	float signProg    = (std::clamp)((appearProgress_ - 0.15f) / 0.85f, 0.0f, 1.0f);
	float barrierScaleY = EaseOutCubic(barrierProg);

	// ネオンの呼吸パルス波（0.0 〜 1.0）
	float pulse = (std::sin(timer_ * 4.2f) + 1.0f) * 0.5f;

	// ヒットストップ中の超高輝度フラッシュ＆微振動
	float hitStopFlash = 0.0f;
	Vector3 jitterOffset = { 0.0f, 0.0f, 0.0f };
	if (isInHitStop_) {
		hitStopTimer_ += rawDt;
		hitStopFlash = (std::sin(timer_ * 65.0f) > 0.0f) ? 1.0f : 0.45f;
		jitterOffset = {
			RandRange(-0.12f, 0.12f),
			RandRange(-0.12f, 0.12f),
			0.0f
		};
	}

	// 2. 光の壁（セキュリティゲート）・コース中央「GOAL」ホログラム（破砕前のみ更新）
	if (!isShattered_) {
		Vector3 barrierPos = {
			basePosition_.x + jitterOffset.x,
			basePosition_.y + jitterOffset.y,
			basePosition_.z
		};

		if (barrierWallObject_) {
			barrierWallObject_->SetPosition(barrierPos);
			barrierWallObject_->SetScale({ 1.0f, (std::max)(0.02f, barrierScaleY), 1.0f });
			barrierWallObject_->SetDrawRatio(barrierProg);
			Vector4 wallCol = isInHitStop_
				? Vector4{ 0.55f + 0.45f * hitStopFlash, 1.0f, 1.0f, 0.98f }
				: Vector4{ 0.05f, 0.85f + 0.15f * pulse, 1.0f, 0.88f * barrierProg };
			barrierWallObject_->SetColor(wallCol);
			barrierWallObject_->Update();
		}

		if (barrierCoreObject_) {
			barrierCoreObject_->SetPosition(barrierPos);
			barrierCoreObject_->SetScale({ 1.0f, (std::max)(0.02f, barrierScaleY), 1.0f });
			barrierCoreObject_->SetDrawRatio(barrierProg);
			Vector4 coreCol = isInHitStop_
				? Vector4{ 1.0f, 0.45f + 0.55f * hitStopFlash, 0.95f, 0.85f }
				: Vector4{ 0.15f + 0.25f * pulse, 0.95f, 1.0f, (0.38f + 0.15f * pulse) * barrierProg };
			barrierCoreObject_->SetColor(coreCol);
			barrierCoreObject_->Update();
		}

		if (scanlineBeamObject_) {
			float sweepY = std::sin(timer_ * 3.2f) * 5.05f * barrierScaleY;
			scanlineBeamObject_->SetPosition({ barrierPos.x, barrierPos.y + sweepY, barrierPos.z });
			scanlineBeamObject_->SetDrawRatio(barrierProg);
			scanlineBeamObject_->SetColor({ 0.80f, 1.0f, 1.0f, 0.92f * barrierProg });
			scanlineBeamObject_->Update();
		}

		// コース中央を塞ぐ巨大「GOAL」ホログラムの更新
		float centerBounce = std::sin(timer_ * 3.0f) * 0.08f;
		float holoFlicker = (appearProgress_ < 0.95f)
			? ((std::sin(timer_ * 55.0f) > -0.3f) ? 1.0f : 0.35f)
			: (0.92f + 0.08f * std::sin(timer_ * 18.0f));

		Vector4 centerGoalColor = isInHitStop_
			? Vector4{ 1.0f, 1.0f, 0.65f + 0.35f * hitStopFlash, 1.0f }
			: Vector4{ 0.30f + 0.40f * pulse, 1.0f, 0.25f + 0.30f * (1.0f - pulse), signProg * holoFlicker };

		for (size_t i = 0; i < centerGoalLetterObjects_.size(); ++i) {
			if (centerGoalLetterObjects_[i] && i < centerLetterXOffsets_.size()) {
				float letterDelay = static_cast<float>(i) * 0.08f;
				float letterProg = (std::clamp)((signProg - letterDelay) / (1.0f - letterDelay), 0.0f, 1.0f);
				float letterScale = EaseOutCubic(letterProg);

				Vector3 letterPos = barrierPos;
				letterPos.x += centerLetterXOffsets_[i];
				letterPos.y += -1.35f + centerBounce;
				centerGoalLetterObjects_[i]->SetPosition(letterPos);
				centerGoalLetterObjects_[i]->SetScale({ 1.0f, (std::max)(0.01f, letterScale), 1.0f });
				centerGoalLetterObjects_[i]->SetDrawRatio(letterProg);
				centerGoalLetterObjects_[i]->SetColor(centerGoalColor);
				centerGoalLetterObjects_[i]->Update();
			}
		}

		// ヒットストップ中のガラス亀裂（クラック）伝播更新
		if (isInHitStop_ && impactCrackObject_) {
			float crackProgress = (std::clamp)(0.25f + (hitStopTimer_ / 0.26f) * 0.75f, 0.0f, 1.0f);
			impactCrackObject_->SetPosition(barrierPos);
			impactCrackObject_->SetDrawRatio(crackProgress);
			impactCrackObject_->SetColor({ 0.90f, 1.0f, 1.0f, 1.0f });
			impactCrackObject_->Update();
		}
	}
	// 3. ゲート破砕後の3Dサイバー発光ボクセル群の物理・回転・GPUインスタンス更新
	else {
		shatterTimer_ += rawDt;

		if (camera_ && !voxels_.empty()) {
			Matrix4x4 vp = MathUtility::Multiply(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());

			std::vector<WireframeObject::InstanceData> outerInstances;
			std::vector<WireframeObject::InstanceData> innerInstances;
			outerInstances.reserve(voxels_.size());
			innerInstances.reserve(voxels_.size());

			for (auto& vx : voxels_) {
				vx.elapsed += dt;
				if (vx.elapsed >= vx.lifeTime) {
					continue;
				}

				float drag = std::pow(0.965f, dt * 60.0f);
				vx.velocity.x *= drag;
				vx.velocity.y = vx.velocity.y * drag - 1.8f * dt;
				vx.velocity.z *= drag;

				vx.position.x += vx.velocity.x * dt;
				vx.position.y += vx.velocity.y * dt;
				vx.position.z += vx.velocity.z * dt;

				vx.rotation.x += vx.angularVelocity.x * dt;
				vx.rotation.y += vx.angularVelocity.y * dt;
				vx.rotation.z += vx.angularVelocity.z * dt;

				float lifeRatio = vx.elapsed / vx.lifeTime;
				float fade = 1.0f - lifeRatio * lifeRatio;
				float sparkle = 0.78f + 0.22f * std::sin(vx.elapsed * 28.0f + vx.position.x * 3.0f);

				Vector3 curScale = {
					vx.baseScale.x * fade,
					vx.baseScale.y * fade,
					vx.baseScale.z * fade
				};

				Matrix4x4 S = MathUtility::MakeScaleMatrix(curScale);
				Matrix4x4 R = MathUtility::MakeRotateMatrix(vx.rotation);
				Matrix4x4 T = MathUtility::MakeTranslateMatrix(vx.position);
				Matrix4x4 worldMat = MathUtility::Multiply(MathUtility::Multiply(S, R), T);

				WireframeObject::InstanceData outInst;
				outInst.WVP = MathUtility::Multiply(worldMat, vp);
				outInst.color = {
					vx.color.x * sparkle,
					vx.color.y * sparkle,
					vx.color.z * sparkle,
					vx.color.w * fade
				};
				outerInstances.push_back(outInst);

				Vector3 innerScale = { curScale.x * 0.52f, curScale.y * 0.52f, curScale.z * 0.52f };
				Matrix4x4 S_in = MathUtility::MakeScaleMatrix(innerScale);
				Matrix4x4 worldIn = MathUtility::Multiply(MathUtility::Multiply(S_in, R), T);

				WireframeObject::InstanceData inInst;
				inInst.WVP = MathUtility::Multiply(worldIn, vp);
				inInst.color = { 1.0f, 1.0f, 1.0f, 0.90f * fade };
				innerInstances.push_back(inInst);
			}

			if (voxelOuterObject_) {
				voxelOuterObject_->UpdateInstances(outerInstances);
			}
			if (voxelInnerObject_) {
				voxelInnerObject_->UpdateInstances(innerInstances);
			}
		}
	}
}

// ----------------------------------------------------
// 描画
// ----------------------------------------------------
void Goal::Draw() {
	// まだゴールに接近しておらず未出現なら描画しない
	if (appearProgress_ <= 0.001f && !isShattered_) {
		return;
	}

	if (!isShattered_) {
		if (barrierCoreObject_) barrierCoreObject_->Draw();
		if (barrierWallObject_) barrierWallObject_->Draw();
		if (scanlineBeamObject_) scanlineBeamObject_->Draw();

		for (auto& letter : centerGoalLetterObjects_) {
			if (letter) letter->Draw();
		}

		if (isInHitStop_ && impactCrackObject_) {
			impactCrackObject_->Draw();
		}
	} else {
		// 破砕後はゲートそのものが完全に消滅し、無数の3Dサイバー発光ボクセルのみが前方へ飛び散る
		if (voxelOuterObject_) voxelOuterObject_->Draw();
		if (voxelInnerObject_) voxelInnerObject_->Draw();
	}
}

void Goal::SetFog(float fogNear, float fogFar) {
	fogNear_ = fogNear;
	fogFar_ = fogFar;

	if (barrierWallObject_) barrierWallObject_->SetFog(fogNear, fogFar);
	if (barrierCoreObject_) barrierCoreObject_->SetFog(fogNear, fogFar);
	if (scanlineBeamObject_) scanlineBeamObject_->SetFog(fogNear, fogFar);
	if (impactCrackObject_) impactCrackObject_->SetFog(fogNear, fogFar);
	if (voxelOuterObject_) voxelOuterObject_->SetFog(fogNear, fogFar);
	if (voxelInnerObject_) voxelInnerObject_->SetFog(fogNear, fogFar);

	for (auto& letter : centerGoalLetterObjects_) {
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
		ImGui::Text("Hologram Cyber Security Goal Gate");
		ImGui::Text("Appear Progress: %.2f | HitStop: %s | Shattered: %s",
			appearProgress_,
			isInHitStop_ ? "YES" : "NO",
			isShattered_ ? "YES" : "NO");
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
	}
	ImGui::End();
#endif
}
