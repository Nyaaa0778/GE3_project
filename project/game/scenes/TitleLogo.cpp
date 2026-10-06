#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "TitleLogo.h"
#include "WireframeObject.h"
#include "WireframeRenderer.h"
#include "Camera.h"
#include "Sprite.h"
#include "MathUtility.h"

#include <cmath>
#include <algorithm>
#include <queue>

namespace {
	float EaseOutQuad(float t) {
		t = (std::max)(0.0f, (std::min)(1.0f, t));
		return 1.0f - (1.0f - t) * (1.0f - t);
	}

	Vector3 CalculateNormalOffset(const Vector3& p0, const Vector3& p1, float thickness) {
		float dx = p1.x - p0.x;
		float dy = p1.y - p0.y;
		float len = std::sqrt(dx * dx + dy * dy);
		if (len < 0.0001f) {
			return {0.0f, thickness, 0.0f};
		}
		return {-dy / len * thickness, dx / len * thickness, 0.0f};
	}
}

TitleLogo::TitleLogo() = default;
TitleLogo::~TitleLogo() = default;

void TitleLogo::Initialize(Camera* customCamera) {
	if (customCamera) {
		activeCamera_ = customCamera;
	} else {
		hudCamera_ = std::make_unique<Camera>();
		hudCamera_->SetFarClip(100.0f);
		hudCamera_->CreateConstantBuffer();
		hudCamera_->SetTranslate({0.0f, 0.0f, -11.0f});
		hudCamera_->SetRotate({0.0f, 0.0f, 0.0f});
		hudCamera_->CalculateMatrix();
		activeCamera_ = hudCamera_.get();
	}

	// 1. メインロゴ背面遮蔽プレート
	backdropSprite_ = std::make_unique<Sprite>();
	backdropSprite_->Initialize("white.png", {640.0f, 180.0f}, {0.5f, 0.5f});
	backdropSprite_->SetSize({780.0f * (rootScale_.x / 0.68f), 155.0f * (rootScale_.y / 0.68f)});
	SetBackdropAlpha(backdropAlpha_);

	// 2. メインロゴ外枠サイバーフレーム [  ]
	frameObject_ = std::make_unique<WireframeObject>();
	frameObject_->Initialize();
	frameObject_->SetCamera(activeCamera_);
	frameObject_->CreateDynamicLineMesh(32);
	frameObject_->SetColor({0.0f, 0.75f, 1.0f, 0.70f});
	frameObject_->SetPosition(rootPosition_);
	frameObject_->SetScale(rootScale_);

	const float fw = 4.6f;
	const float fh = 1.10f;
	const float corner = 0.35f;
	std::vector<std::pair<Vector3, Vector3>> frameLines = {
		{{-fw + corner,  fh, 0.0f}, { fw - corner,  fh, 0.0f}},
		{{-fw + corner, -fh, 0.0f}, { fw - corner, -fh, 0.0f}},
		{{-fw,  fh - corner, 0.0f}, {-fw,  fh, 0.0f}},
		{{-fw,  fh, 0.0f}, {-fw + corner,  fh, 0.0f}},
		{{ fw,  fh - corner, 0.0f}, { fw,  fh, 0.0f}},
		{{ fw,  fh, 0.0f}, { fw - corner,  fh, 0.0f}},
		{{-fw, -fh + corner, 0.0f}, {-fw, -fh, 0.0f}},
		{{-fw, -fh, 0.0f}, {-fw + corner, -fh, 0.0f}},
		{{ fw, -fh + corner, 0.0f}, { fw, -fh, 0.0f}},
		{{ fw, -fh, 0.0f}, { fw - corner, -fh, 0.0f}},
		{{-1.2f, -fh - 0.07f, 0.0f}, { 1.2f, -fh - 0.07f, 0.0f}}
	};
	frameObject_->UpdateDynamicLines(frameLines);

	// 3. サブテキスト「PRESS SPACE TO START」の背面遮蔽プレート
	promptBackdrop_ = std::make_unique<Sprite>();
	promptBackdrop_->Initialize("white.png", {640.0f, 435.0f}, {0.5f, 0.5f});
	promptBackdrop_->SetSize({640.0f * (rootScale_.x / 0.68f), 42.0f * (rootScale_.y / 0.68f)});
	promptBackdrop_->SetColor({0.010f, 0.015f, 0.030f, 0.75f});

	// サブテキストの外枠アクセント（左右の小さなブラケット < ... >）
	promptFrameObject_ = std::make_unique<WireframeObject>();
	promptFrameObject_->Initialize();
	promptFrameObject_->SetCamera(activeCamera_);
	promptFrameObject_->CreateDynamicLineMesh(16);
	promptFrameObject_->SetColor({0.0f, 0.85f, 1.0f, 0.60f});
	promptFrameObject_->SetPosition(promptPosition_);
	promptFrameObject_->SetScale(rootScale_);

	const float pfw = 3.35f;
	const float pfh = 0.28f;
	const float pcorner = 0.12f;
	std::vector<std::pair<Vector3, Vector3>> pframeLines = {
		{{-pfw,  pfh - pcorner, 0.0f}, {-pfw,  pfh, 0.0f}},
		{{-pfw,  pfh, 0.0f}, {-pfw + pcorner,  pfh, 0.0f}},
		{{ pfw,  pfh - pcorner, 0.0f}, { pfw,  pfh, 0.0f}},
		{{ pfw,  pfh, 0.0f}, { pfw - pcorner,  pfh, 0.0f}},
		{{-pfw, -pfh + pcorner, 0.0f}, {-pfw, -pfh, 0.0f}},
		{{-pfw, -pfh, 0.0f}, {-pfw + pcorner, -pfh, 0.0f}},
		{{ pfw, -pfh + pcorner, 0.0f}, { pfw, -pfh, 0.0f}},
		{{ pfw, -pfh, 0.0f}, { pfw - pcorner, -pfh, 0.0f}},
	};
	promptFrameObject_->UpdateDynamicLines(pframeLines);

	// 4. メインロゴメッシュ構築
	BuildLetterMeshes();

	// 5. サブテキスト「PRESS SPACE TO START」メッシュ構築
	BuildPromptMeshes();

	// 6. アニメーション初期化
	ResetAnimation();

	// 位置・スケール・回転の反映
	SetPosition(rootPosition_);
	SetScale(rootScale_);
	SetRotation(rootRotation_);
	SetPromptPosition(promptPosition_);
}

void TitleLogo::SetRotation(const Vector3& rotation) {
	rootRotation_ = rotation;
	if (frameObject_) {
		frameObject_->SetRotation(rootRotation_);
	}
	for (auto& l : letters_) {
		if (l.wireObject) {
			l.wireObject->SetRotation(rootRotation_);
		}
	}
}

void TitleLogo::SetBackdropAlpha(float alpha) {
	backdropAlpha_ = alpha;
	if (backdropSprite_) {
		backdropSprite_->SetColor({0.010f, 0.015f, 0.030f, backdropAlpha_});
	}
}

void TitleLogo::SetPosition(const Vector3& position) {
	rootPosition_ = position;
	if (frameObject_) {
		frameObject_->SetPosition(rootPosition_);
	}
	for (auto& l : letters_) {
		if (l.wireObject) {
			Vector3 pos = rootPosition_;
			pos.x += l.xOffset * rootScale_.x;
			l.wireObject->SetPosition(pos);
		}
	}

	if (backdropSprite_) {
		const float halfViewY = 11.0f * std::tan(0.45f * 0.5f);
		const float halfViewX = halfViewY * (1280.0f / 720.0f);
		float scrX = 640.0f + (rootPosition_.x / halfViewX) * 640.0f;
		float scrY = 360.0f - (rootPosition_.y / halfViewY) * 360.0f;
		backdropSprite_->SetPosition({scrX, scrY});
	}
}

void TitleLogo::SetPromptPosition(const Vector3& position) {
	promptPosition_ = position;
	if (promptFrameObject_) {
		promptFrameObject_->SetPosition(promptPosition_);
	}
	for (auto& l : promptLetters_) {
		if (l.wireObject) {
			Vector3 pos = promptPosition_;
			pos.x += l.xOffset * rootScale_.x;
			l.wireObject->SetPosition(pos);
		}
	}

	if (promptBackdrop_) {
		const float halfViewY = 11.0f * std::tan(0.45f * 0.5f);
		const float halfViewX = halfViewY * (1280.0f / 720.0f);
		float scrX = 640.0f + (promptPosition_.x / halfViewX) * 640.0f;
		float scrY = 360.0f - (promptPosition_.y / halfViewY) * 360.0f;
		promptBackdrop_->SetPosition({scrX, scrY});
	}
}

void TitleLogo::SetScale(const Vector3& scale) {
	rootScale_ = scale;
	if (frameObject_) {
		frameObject_->SetScale(rootScale_);
	}
	for (auto& l : letters_) {
		if (l.wireObject) {
			l.wireObject->SetScale(rootScale_);
		}
	}
	if (promptFrameObject_) {
		promptFrameObject_->SetScale(rootScale_);
	}
	for (auto& l : promptLetters_) {
		if (l.wireObject) {
			l.wireObject->SetScale(rootScale_);
		}
	}
	if (backdropSprite_) {
		backdropSprite_->SetSize({780.0f * (rootScale_.x / 0.68f), 155.0f * (rootScale_.y / 0.68f)});
	}
	if (promptBackdrop_) {
		promptBackdrop_->SetSize({640.0f * (rootScale_.x / 0.68f), 42.0f * (rootScale_.y / 0.68f)});
	}
	SetPosition(rootPosition_);
	SetPromptPosition(promptPosition_);
}

void TitleLogo::BuildLetterMeshes() {
	letters_.clear();
	const std::string text = "CYBERAIL";
	letters_.resize(text.size());

	const float letterH = 1.40f;
	const float letterT = 0.22f;
	const float spacing = 0.22f;

	float totalWidth = 0.0f;
	for (size_t i = 0; i < text.size(); ++i) {
		letters_[i].character = text[i];
		// 文字ごとの標準幅
		float w = 1.00f;
		if (text[i] == 'I') w = 0.42f;
		else if (text[i] == 'E') w = 0.90f;
		else if (text[i] == 'L') w = 0.85f;
		else if (text[i] == 'A') w = 1.05f;

		GenerateCharMesh(text[i], w, letterH, letterT, letters_[i]);
		totalWidth += letters_[i].width;
		if (i + 1 < text.size()) totalWidth += spacing;
	}

	float currentX = -totalWidth * 0.5f;
	const float yCenterOffset = -letterH * 0.5f;
	const float halfDepth = extrusionDepth_ * 0.5f;

	for (size_t i = 0; i < letters_.size(); ++i) {
		auto& l = letters_[i];
		l.xOffset = currentX;
		currentX += l.width + spacing;

		// 2Dの頂点・エッジから、3D立体押し出し（前面 + 背面 + 奥行き連結梁）を構築
		std::vector<Vector3> frontVerts = l.localVertices;
		std::vector<std::pair<int, int>> frontEdges = l.baseEdges;
		const size_t N = frontVerts.size();

		l.localVertices.clear();
		l.localVertices.reserve(N * 2);

		// 1. 前面頂点 (Z = -halfDepth: 手前に迫り出す)
		for (const auto& v : frontVerts) {
			l.localVertices.push_back({v.x, v.y + yCenterOffset, -halfDepth});
		}
		// 2. 背面頂点 (Z = +halfDepth: 奥へ沈む)
		for (const auto& v : frontVerts) {
			l.localVertices.push_back({v.x, v.y + yCenterOffset, halfDepth});
		}

		l.baseEdges.clear();
		l.baseEdges.reserve(frontEdges.size() * 2 + N);

		// 前面エッジ (0..N-1)
		for (const auto& e : frontEdges) {
			l.baseEdges.push_back(e);
		}
		// 背面エッジ (N..2N-1)
		for (const auto& e : frontEdges) {
			l.baseEdges.push_back({e.first + static_cast<int>(N), e.second + static_cast<int>(N)});
		}
		// 奥行きを繋ぐ立体梁 (Struts) (i <-> i+N)
		for (size_t k = 0; k < N; ++k) {
			l.baseEdges.push_back({static_cast<int>(k), static_cast<int>(k + N)});
		}

		l.baseColor = {0.15f, 0.95f, 1.0f, 1.0f}; // 鮮烈エレクトリックシアン
		l.wireObject = std::make_unique<WireframeObject>();
		l.wireObject->Initialize();
		l.wireObject->SetCamera(activeCamera_);
		// 前面ダブルライン + 背面・梁用の十分なバッファを確保
		l.wireObject->CreateDynamicLineMesh(static_cast<uint32_t>(l.baseEdges.size() * 2));
		l.wireObject->SetColor(l.baseColor);

		Vector3 pos = rootPosition_;
		pos.x += l.xOffset * rootScale_.x;
		l.wireObject->SetPosition(pos);
		l.wireObject->SetScale(rootScale_);
		l.wireObject->SetRotation(rootRotation_);
	}
}

void TitleLogo::BuildPromptMeshes() {
	promptLetters_.clear();
	const std::string text = "PRESS SPACE TO START";
	promptLetters_.resize(text.size());

	const float letterH = 0.30f;
	const float letterT = 0.052f;
	const float spacing = 0.075f;

	float totalWidth = 0.0f;
	for (size_t i = 0; i < text.size(); ++i) {
		char c = text[i];
		promptLetters_[i].character = c;
		float w = 0.22f;
		if (c == ' ') w = 0.15f;
		else if (c == 'I') w = 0.11f;
		else if (c == 'M' || c == 'W') w = 0.28f;

		GenerateCharMesh(c, w, letterH, letterT, promptLetters_[i]);
		totalWidth += promptLetters_[i].width;
		if (i + 1 < text.size()) totalWidth += spacing;
	}

	float currentX = -totalWidth * 0.5f;
	const float yCenterOffset = -letterH * 0.5f;

	for (size_t i = 0; i < promptLetters_.size(); ++i) {
		auto& l = promptLetters_[i];
		l.xOffset = currentX;
		currentX += l.width + spacing;

		for (auto& v : l.localVertices) {
			v.y += yCenterOffset;
		}

		// サブテキストはクールなホワイトシアン
		l.baseColor = {0.70f, 0.95f, 1.0f, 1.0f};

		if (!l.baseEdges.empty()) {
			l.wireObject = std::make_unique<WireframeObject>();
			l.wireObject->Initialize();
			l.wireObject->SetCamera(activeCamera_);
			l.wireObject->CreateDynamicLineMesh(static_cast<uint32_t>(l.baseEdges.size() * 2));
			l.wireObject->SetColor(l.baseColor);

			Vector3 pos = promptPosition_;
			pos.x += l.xOffset * rootScale_.x;
			l.wireObject->SetPosition(pos);
			l.wireObject->SetScale(rootScale_);
		}
	}
}

void TitleLogo::SetupGrowthAnimation() {
	animTime_ = 0.0f;
	promptAnimTime_ = 0.0f;
	pulseTimer_ = 0.0f;
	promptBlinkTimer_ = 0.0f;
	isAllBuilt_ = false;
	isPromptBuilt_ = false;

	// メインロゴ形成スケジュール
	for (size_t i = 0; i < letters_.size(); ++i) {
		auto& l = letters_[i];
		l.startDelay = static_cast<float>(i) * 0.14f;
		l.isComplete = false;
		l.flashTimer = 0.0f;
		l.edgeGrowths.clear();
		l.edgeGrowths.reserve(l.baseEdges.size());

		if (l.localVertices.empty() || l.baseEdges.empty()) continue;

		float minY = l.localVertices[0].y;
		for (const auto& v : l.localVertices) {
			if (v.y < minY) minY = v.y;
		}

		std::vector<bool> vertexReached(l.localVertices.size(), false);
		std::vector<float> vertexReachedTime(l.localVertices.size(), 0.0f);
		std::vector<bool> edgeScheduled(l.baseEdges.size(), false);
		std::vector<int> frontier;

		for (size_t vIdx = 0; vIdx < l.localVertices.size(); ++vIdx) {
			if (std::abs(l.localVertices[vIdx].y - minY) < 0.10f) {
				vertexReached[vIdx] = true;
				vertexReachedTime[vIdx] = 0.0f;
				frontier.push_back(static_cast<int>(vIdx));
			}
		}

		if (frontier.empty()) {
			vertexReached[0] = true;
			vertexReachedTime[0] = 0.0f;
			frontier.push_back(0);
		}

		while (!frontier.empty()) {
			std::sort(frontier.begin(), frontier.end(), [&](int a, int b) {
				return vertexReachedTime[a] < vertexReachedTime[b];
			});
			int curr = frontier.front();
			frontier.erase(frontier.begin());

			for (size_t eIdx = 0; eIdx < l.baseEdges.size(); ++eIdx) {
				if (edgeScheduled[eIdx]) continue;

				int u = l.baseEdges[eIdx].first;
				int v = l.baseEdges[eIdx].second;
				if (u != curr && v != curr) continue;

				edgeScheduled[eIdx] = true;
				int nextVertex = (u == curr) ? v : u;

				const float duration = 0.14f;
				float edgeStart = vertexReachedTime[curr];
				float edgeEnd = edgeStart + duration;

				EdgeGrowth eg;
				eg.fromVertex = curr;
				eg.toVertex = nextVertex;
				eg.startTime = edgeStart;
				eg.duration = duration;
				l.edgeGrowths.push_back(eg);

				if (!vertexReached[nextVertex]) {
					vertexReached[nextVertex] = true;
					vertexReachedTime[nextVertex] = edgeEnd;
					frontier.push_back(nextVertex);
				}
			}
		}

		for (size_t eIdx = 0; eIdx < l.baseEdges.size(); ++eIdx) {
			if (!edgeScheduled[eIdx]) {
				edgeScheduled[eIdx] = true;
				int u = l.baseEdges[eIdx].first;
				int v = l.baseEdges[eIdx].second;
				int fromV = (vertexReachedTime[u] <= vertexReachedTime[v]) ? u : v;
				int toV = (fromV == u) ? v : u;
				float edgeStart = (std::max)(vertexReachedTime[u], vertexReachedTime[v]);
				const float duration = 0.14f;

				EdgeGrowth eg;
				eg.fromVertex = fromV;
				eg.toVertex = toV;
				eg.startTime = edgeStart;
				eg.duration = duration;
				l.edgeGrowths.push_back(eg);
			}
		}

		if (l.wireObject) {
			l.wireObject->UpdateDynamicLines({});
		}
	}

	// サブプロンプト形成スケジュール（メインロゴ完成間近の 1.1s 後から高速に展開）
	const float promptBaseDelay = 1.10f;
	for (size_t i = 0; i < promptLetters_.size(); ++i) {
		auto& l = promptLetters_[i];
		l.startDelay = promptBaseDelay + static_cast<float>(i) * 0.045f;
		l.isComplete = false;
		l.flashTimer = 0.0f;
		l.edgeGrowths.clear();
		l.edgeGrowths.reserve(l.baseEdges.size());

		if (l.localVertices.empty() || l.baseEdges.empty()) continue;

		float minY = l.localVertices[0].y;
		for (const auto& v : l.localVertices) {
			if (v.y < minY) minY = v.y;
		}

		std::vector<bool> vertexReached(l.localVertices.size(), false);
		std::vector<float> vertexReachedTime(l.localVertices.size(), 0.0f);
		std::vector<bool> edgeScheduled(l.baseEdges.size(), false);
		std::vector<int> frontier;

		for (size_t vIdx = 0; vIdx < l.localVertices.size(); ++vIdx) {
			if (std::abs(l.localVertices[vIdx].y - minY) < 0.05f) {
				vertexReached[vIdx] = true;
				vertexReachedTime[vIdx] = 0.0f;
				frontier.push_back(static_cast<int>(vIdx));
			}
		}

		if (frontier.empty()) {
			vertexReached[0] = true;
			vertexReachedTime[0] = 0.0f;
			frontier.push_back(0);
		}

		while (!frontier.empty()) {
			std::sort(frontier.begin(), frontier.end(), [&](int a, int b) {
				return vertexReachedTime[a] < vertexReachedTime[b];
			});
			int curr = frontier.front();
			frontier.erase(frontier.begin());

			for (size_t eIdx = 0; eIdx < l.baseEdges.size(); ++eIdx) {
				if (edgeScheduled[eIdx]) continue;

				int u = l.baseEdges[eIdx].first;
				int v = l.baseEdges[eIdx].second;
				if (u != curr && v != curr) continue;

				edgeScheduled[eIdx] = true;
				int nextVertex = (u == curr) ? v : u;

				const float duration = 0.09f;
				float edgeStart = vertexReachedTime[curr];
				float edgeEnd = edgeStart + duration;

				EdgeGrowth eg;
				eg.fromVertex = curr;
				eg.toVertex = nextVertex;
				eg.startTime = edgeStart;
				eg.duration = duration;
				l.edgeGrowths.push_back(eg);

				if (!vertexReached[nextVertex]) {
					vertexReached[nextVertex] = true;
					vertexReachedTime[nextVertex] = edgeEnd;
					frontier.push_back(nextVertex);
				}
			}
		}

		for (size_t eIdx = 0; eIdx < l.baseEdges.size(); ++eIdx) {
			if (!edgeScheduled[eIdx]) {
				edgeScheduled[eIdx] = true;
				int u = l.baseEdges[eIdx].first;
				int v = l.baseEdges[eIdx].second;
				int fromV = (vertexReachedTime[u] <= vertexReachedTime[v]) ? u : v;
				int toV = (fromV == u) ? v : u;
				float edgeStart = (std::max)(vertexReachedTime[u], vertexReachedTime[v]);
				const float duration = 0.09f;

				EdgeGrowth eg;
				eg.fromVertex = fromV;
				eg.toVertex = toV;
				eg.startTime = edgeStart;
				eg.duration = duration;
				l.edgeGrowths.push_back(eg);
			}
		}

		if (l.wireObject) {
			l.wireObject->UpdateDynamicLines({});
		}
	}
}

void TitleLogo::ResetAnimation() {
	SetupGrowthAnimation();
}

void TitleLogo::UpdateGrowthAnimation() {
	const float dt = 0.016f;
	animTime_ += dt;

	const float lineThickness = 0.009f;
	std::vector<std::pair<Vector3, Vector3>> activeLines;
	bool allDone = true;

	for (auto& l : letters_) {
		float localTime = animTime_ - l.startDelay;
		if (localTime < 0.0f) {
			allDone = false;
			continue;
		}

		if (l.isComplete) {
			if (l.flashTimer > 0.0f) {
				l.flashTimer -= dt;
				float t = std::clamp(l.flashTimer / 0.25f, 0.0f, 1.0f);
				Vector4 c = {
					std::lerp(l.baseColor.x, 2.2f, t),
					std::lerp(l.baseColor.y, 2.2f, t),
					std::lerp(l.baseColor.z, 2.4f, t),
					1.0f
				};
				l.wireObject->SetColor(c);
			}
			continue;
		}

		activeLines.clear();
		bool letterEdgesDone = true;

		for (const auto& eg : l.edgeGrowths) {
			if (localTime < eg.startTime) {
				letterEdgesDone = false;
				continue;
			}

			const Vector3& p0 = l.localVertices[eg.fromVertex];
			const Vector3& p1 = l.localVertices[eg.toVertex];
			// 前面ライン (Z < -0.001) は太いダブルラインでクッキリ強調、梁や奥面はシングルラインでスッキリ立体化
			bool isFrontLine = (p0.z < -0.001f && p1.z < -0.001f);

			if (localTime >= eg.startTime + eg.duration) {
				if (isFrontLine) {
					Vector3 n = CalculateNormalOffset(p0, p1, lineThickness);
					activeLines.push_back({MathUtility::Add(p0, n), MathUtility::Add(p1, n)});
					activeLines.push_back({MathUtility::Subtract(p0, n), MathUtility::Subtract(p1, n)});
				} else {
					activeLines.push_back({p0, p1});
				}
			} else {
				letterEdgesDone = false;
				float t = (localTime - eg.startTime) / eg.duration;
				float progress = EaseOutQuad(t);
				Vector3 currentEnd = MathUtility::Add(p0, MathUtility::Multiply(MathUtility::Subtract(p1, p0), progress));

				if (isFrontLine) {
					Vector3 n = CalculateNormalOffset(p0, currentEnd, lineThickness);
					activeLines.push_back({MathUtility::Add(p0, n), MathUtility::Add(currentEnd, n)});
					activeLines.push_back({MathUtility::Subtract(p0, n), MathUtility::Subtract(currentEnd, n)});
				} else {
					activeLines.push_back({p0, currentEnd});
				}
			}
		}

		if (letterEdgesDone) {
			l.isComplete = true;
			l.flashTimer = 0.25f;
			activeLines.clear();
			for (const auto& eg : l.edgeGrowths) {
				const Vector3& p0 = l.localVertices[eg.fromVertex];
				const Vector3& p1 = l.localVertices[eg.toVertex];
				bool isFrontLine = (p0.z < -0.001f && p1.z < -0.001f);
				if (isFrontLine) {
					Vector3 n = CalculateNormalOffset(p0, p1, lineThickness);
					activeLines.push_back({MathUtility::Add(p0, n), MathUtility::Add(p1, n)});
					activeLines.push_back({MathUtility::Subtract(p0, n), MathUtility::Subtract(p1, n)});
				} else {
					activeLines.push_back({p0, p1});
				}
			}
		} else {
			allDone = false;
		}

		if (l.wireObject) {
			l.wireObject->UpdateDynamicLines(activeLines);
		}
	}

	isAllBuilt_ = allDone;

	if (isAllBuilt_) {
		pulseTimer_ += dt;
		float pulse = 0.90f + 0.10f * std::sin(pulseTimer_ * 3.5f);
		for (auto& l : letters_) {
			if (l.flashTimer <= 0.0f) {
				l.wireObject->SetColor({
					l.baseColor.x * pulse,
					l.baseColor.y * pulse,
					l.baseColor.z * pulse,
					1.0f
				});
			}
		}
	}
}

void TitleLogo::UpdatePromptAnimation() {
	const float dt = 0.016f;
	promptAnimTime_ += dt;

	const float lineThickness = 0.0032f; // 小さめサブテキスト用の肉厚オフセット
	std::vector<std::pair<Vector3, Vector3>> activeLines;
	bool allDone = true;

	for (auto& l : promptLetters_) {
		if (l.baseEdges.empty()) continue; // スペース

		float localTime = promptAnimTime_ - l.startDelay;
		if (localTime < 0.0f) {
			allDone = false;
			continue;
		}

		if (l.isComplete) {
			if (l.flashTimer > 0.0f) {
				l.flashTimer -= dt;
				float t = std::clamp(l.flashTimer / 0.20f, 0.0f, 1.0f);
				Vector4 c = {
					std::lerp(l.baseColor.x, 2.0f, t),
					std::lerp(l.baseColor.y, 2.0f, t),
					std::lerp(l.baseColor.z, 2.0f, t),
					1.0f
				};
				l.wireObject->SetColor(c);
			}
			continue;
		}

		activeLines.clear();
		bool letterDone = true;

		for (const auto& eg : l.edgeGrowths) {
			if (localTime < eg.startTime) {
				letterDone = false;
				continue;
			}

			const Vector3& p0 = l.localVertices[eg.fromVertex];
			const Vector3& p1 = l.localVertices[eg.toVertex];

			if (localTime >= eg.startTime + eg.duration) {
				Vector3 n = CalculateNormalOffset(p0, p1, lineThickness);
				activeLines.push_back({MathUtility::Add(p0, n), MathUtility::Add(p1, n)});
				activeLines.push_back({MathUtility::Subtract(p0, n), MathUtility::Subtract(p1, n)});
			} else {
				letterDone = false;
				float t = (localTime - eg.startTime) / eg.duration;
				float progress = EaseOutQuad(t);
				Vector3 currentEnd = MathUtility::Add(p0, MathUtility::Multiply(MathUtility::Subtract(p1, p0), progress));

				Vector3 n = CalculateNormalOffset(p0, currentEnd, lineThickness);
				activeLines.push_back({MathUtility::Add(p0, n), MathUtility::Add(currentEnd, n)});
				activeLines.push_back({MathUtility::Subtract(p0, n), MathUtility::Subtract(currentEnd, n)});
			}
		}

		if (letterDone) {
			l.isComplete = true;
			l.flashTimer = 0.20f;
			activeLines.clear();
			for (const auto& eg : l.edgeGrowths) {
				const Vector3& p0 = l.localVertices[eg.fromVertex];
				const Vector3& p1 = l.localVertices[eg.toVertex];
				Vector3 n = CalculateNormalOffset(p0, p1, lineThickness);
				activeLines.push_back({MathUtility::Add(p0, n), MathUtility::Add(p1, n)});
				activeLines.push_back({MathUtility::Subtract(p0, n), MathUtility::Subtract(p1, n)});
			}
		} else {
			allDone = false;
		}

		if (l.wireObject) {
			l.wireObject->UpdateDynamicLines(activeLines);
		}
	}

	isPromptBuilt_ = allDone;

	// サブプロンプト完成後は、プレイヤーにSPACE入力を促す点滅アニメーション
	if (isPromptBuilt_) {
		promptBlinkTimer_ += dt;
		// 1.2秒周期の滑らかな呼吸点滅 (min 0.25 〜 max 1.0)
		float blink = 0.25f + 0.75f * (0.5f + 0.5f * std::sin(promptBlinkTimer_ * 4.2f));
		for (auto& l : promptLetters_) {
			if (l.wireObject && l.flashTimer <= 0.0f) {
				l.wireObject->SetColor({
					l.baseColor.x * blink,
					l.baseColor.y * blink,
					l.baseColor.z * blink,
					1.0f
				});
			}
		}
		if (promptFrameObject_) {
			promptFrameObject_->SetColor({0.0f, 0.85f, 1.0f, 0.50f * blink});
		}
	}
}

void TitleLogo::Update() {
	if (backdropSprite_ && enableBackdrop_) {
		backdropSprite_->Update();
	}

	// 浮遊アニメーション（立体ロゴが空間にふわふわと浮き出る）
	if (enableHover_) {
		float hoverY = std::sin(pulseTimer_ * 1.5f) * 0.035f;
		float hoverPitch = rootRotation_.x + std::sin(pulseTimer_ * 1.2f) * 0.020f;
		float hoverYaw = rootRotation_.y + std::cos(pulseTimer_ * 1.0f) * 0.015f;

		Vector3 curPos = rootPosition_;
		curPos.y += hoverY;
		Vector3 curRot = {hoverPitch, hoverYaw, rootRotation_.z};

		if (frameObject_) {
			frameObject_->SetPosition(curPos);
			frameObject_->SetRotation(curRot);
		}
		for (auto& l : letters_) {
			if (l.wireObject) {
				Vector3 pos = curPos;
				pos.x += l.xOffset * rootScale_.x;
				l.wireObject->SetPosition(pos);
				l.wireObject->SetRotation(curRot);
			}
		}
	}

	if (frameObject_) {
		frameObject_->Update();
	}

	if (promptBackdrop_ && enableBackdrop_) {
		promptBackdrop_->Update();
	}
	if (promptFrameObject_) {
		promptFrameObject_->Update();
	}

	UpdateGrowthAnimation();
	UpdatePromptAnimation();

	for (auto& l : letters_) {
		if (l.wireObject) {
			l.wireObject->Update();
		}
	}
	for (auto& l : promptLetters_) {
		if (l.wireObject) {
			l.wireObject->Update();
		}
	}
}

void TitleLogo::Draw() {
	// 1. メインロゴ背景遮蔽プレート
	if (backdropSprite_ && enableBackdrop_) {
		backdropSprite_->Draw();
	}
	// 2. メインロゴ外枠フレーム [  ]
	if (frameObject_) {
		frameObject_->Draw();
	}
	// 3. メインロゴ文字
	for (auto& l : letters_) {
		if (l.wireObject) {
			l.wireObject->Draw();
		}
	}

	// 4. サブテキスト背景遮蔽プレート
	if (promptBackdrop_ && enableBackdrop_) {
		promptBackdrop_->Draw();
	}
	// 5. サブテキスト外枠フレーム <  >
	if (promptFrameObject_) {
		promptFrameObject_->Draw();
	}
	// 6. サブテキスト文字「PRESS SPACE TO START」
	for (auto& l : promptLetters_) {
		if (l.wireObject) {
			l.wireObject->Draw();
		}
	}
}

// ============================================================================
// 任意のサイズに対応した洗練サイバーブロックフォント生成関数
// ============================================================================

void TitleLogo::GenerateCharMesh(char c, float w, float h, float t, LetterData& l) {
	l.width = w;
	l.localVertices.clear();
	l.baseEdges.clear();

	const float cut = t * 1.1f; // 角の45度面取り量

	switch (c) {
	case 'C': {
		l.localVertices = {
			// 外周 (0..5)
			{w, h, 0.0f}, {cut, h, 0.0f}, {0.0f, h - cut, 0.0f},
			{0.0f, cut, 0.0f}, {cut, 0.0f, 0.0f}, {w, 0.0f, 0.0f},
			// 内周 (6..11)
			{w, t, 0.0f}, {cut + t * 0.2f, t, 0.0f}, {t, cut + t * 0.2f, 0.0f},
			{t, h - cut - t * 0.2f, 0.0f}, {cut + t * 0.2f, h - t, 0.0f}, {w, h - t, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5},
			{6, 7}, {7, 8}, {8, 9}, {9, 10}, {10, 11},
			{0, 11}, {5, 6}
		};
		break;
	}
	case 'Y': {
		l.localVertices = {
			{w * 0.5f - t * 0.5f, 0.0f, 0.0f}, {w * 0.5f + t * 0.5f, 0.0f, 0.0f},
			{w * 0.5f - t * 0.5f, h * 0.45f, 0.0f}, {w * 0.5f + t * 0.5f, h * 0.45f, 0.0f},
			{0.0f, h, 0.0f}, {t, h, 0.0f},
			{w - t, h, 0.0f}, {w, h, 0.0f},
			{w * 0.5f, h * 0.65f, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {0, 2}, {1, 3},
			{2, 4}, {4, 5}, {5, 8},
			{3, 7}, {7, 6}, {6, 8},
			{8, 2}, {8, 3}
		};
		break;
	}
	case 'B': {
		const float midY = h * 0.52f;
		l.localVertices = {
			{0.0f, 0.0f, 0.0f}, {0.0f, h, 0.0f}, {w * 0.72f, h, 0.0f},
			{w, h - cut, 0.0f}, {w, midY + cut * 0.5f, 0.0f}, {w * 0.75f, midY, 0.0f},
			{w, midY - cut * 0.5f, 0.0f}, {w, cut, 0.0f}, {w * 0.72f, 0.0f, 0.0f},
			// 上窓
			{t, h - t, 0.0f}, {w * 0.68f, h - t, 0.0f},
			{w * 0.76f, h * 0.75f, 0.0f}, {w * 0.68f, midY + t * 0.5f, 0.0f}, {t, midY + t * 0.5f, 0.0f},
			// 下窓
			{t, midY - t * 0.5f, 0.0f}, {w * 0.68f, midY - t * 0.5f, 0.0f},
			{w * 0.76f, h * 0.25f, 0.0f}, {w * 0.68f, t, 0.0f}, {t, t, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}, {7, 8}, {8, 0},
			{9, 10}, {10, 11}, {11, 12}, {12, 13}, {13, 9},
			{14, 15}, {15, 16}, {16, 17}, {17, 18}, {18, 14}
		};
		break;
	}
	case 'E': {
		l.localVertices = {
			{0.0f, 0.0f, 0.0f}, {0.0f, h, 0.0f}, {w, h, 0.0f}, {w, h - t, 0.0f},
			{t, h - t, 0.0f}, {t, h * 0.58f, 0.0f}, {w * 0.82f, h * 0.58f, 0.0f},
			{w * 0.82f, h * 0.42f, 0.0f}, {t, h * 0.42f, 0.0f}, {t, t, 0.0f},
			{w, t, 0.0f}, {w, 0.0f, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6},
			{6, 7}, {7, 8}, {8, 9}, {9, 10}, {10, 11}, {11, 0}
		};
		break;
	}
	case 'R': {
		const float midY = h * 0.52f;
		l.localVertices = {
			{0.0f, 0.0f, 0.0f}, {0.0f, h, 0.0f}, {w * 0.72f, h, 0.0f},
			{w, h - cut, 0.0f}, {w, midY + cut * 0.5f, 0.0f}, {w * 0.72f, midY, 0.0f},
			{w, 0.0f, 0.0f}, {w - t - cut * 0.2f, 0.0f, 0.0f}, {w * 0.54f, midY, 0.0f},
			{t, midY, 0.0f}, {t, 0.0f, 0.0f},
			// 上窓
			{t, h - t, 0.0f}, {w * 0.68f, h - t, 0.0f},
			{w * 0.76f, h * 0.75f, 0.0f}, {w * 0.68f, midY + t * 0.5f, 0.0f}, {t, midY + t * 0.5f, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6},
			{6, 7}, {7, 8}, {8, 9}, {9, 10}, {10, 0},
			{11, 12}, {12, 13}, {13, 14}, {14, 15}, {15, 11}
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
	case 'I': {
		const float ser = t * 0.8f;
		l.localVertices = {
			{0.0f, 0.0f, 0.0f}, {0.0f, ser, 0.0f}, {w * 0.25f, ser, 0.0f},
			{w * 0.25f, h - ser, 0.0f}, {0.0f, h - ser, 0.0f}, {0.0f, h, 0.0f},
			{w, h, 0.0f}, {w, h - ser, 0.0f}, {w * 0.75f, h - ser, 0.0f},
			{w * 0.75f, ser, 0.0f}, {w, ser, 0.0f}, {w, 0.0f, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6},
			{6, 7}, {7, 8}, {8, 9}, {9, 10}, {10, 11}, {11, 0}
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
	case 'P': {
		const float midY = h * 0.46f;
		l.localVertices = {
			{0.0f, 0.0f, 0.0f}, {0.0f, h, 0.0f}, {w * 0.72f, h, 0.0f},
			{w, h - cut, 0.0f}, {w, midY + cut * 0.5f, 0.0f}, {w * 0.72f, midY, 0.0f},
			{t, midY, 0.0f}, {t, 0.0f, 0.0f},
			// 中窓
			{t, h - t, 0.0f}, {w * 0.68f, h - t, 0.0f},
			{w * 0.76f, h * 0.73f, 0.0f}, {w * 0.68f, midY + t * 0.5f, 0.0f}, {t, midY + t * 0.5f, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}, {7, 0},
			{8, 9}, {9, 10}, {10, 11}, {11, 12}, {12, 8}
		};
		break;
	}
	case 'S': {
		const float midY = h * 0.50f;
		const float ht = t * 0.50f;
		l.localVertices = {
			{w, h, 0.0f},               // 0: 上バー右上
			{0.0f, h, 0.0f},            // 1: 上バー左上
			{0.0f, midY - ht, 0.0f},    // 2: 左上垂直バー外側（中央下まで）
			{w - t, midY - ht, 0.0f},   // 3: 中央バー底面
			{w - t, t, 0.0f},           // 4: 右下垂直バー内側
			{0.0f, t, 0.0f},            // 5: 下バー左上
			{0.0f, 0.0f, 0.0f},         // 6: 下バー左下
			{w, 0.0f, 0.0f},            // 7: 下バー右下
			{w, midY + ht, 0.0f},       // 8: 右下垂直バー外側（中央上まで）
			{t, midY + ht, 0.0f},       // 9: 中央バー天面
			{t, h - t, 0.0f},           // 10: 左上垂直バー内側
			{w, h - t, 0.0f}            // 11: 上バー右下
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6},
			{6, 7}, {7, 8}, {8, 9}, {9, 10}, {10, 11}, {11, 0}
		};
		break;
	}
	case 'T': {
		l.localVertices = {
			{0.0f, h, 0.0f}, {w, h, 0.0f}, {w, h - t, 0.0f},
			{w * 0.5f + t * 0.5f, h - t, 0.0f}, {w * 0.5f + t * 0.5f, 0.0f, 0.0f},
			{w * 0.5f - t * 0.5f, 0.0f, 0.0f}, {w * 0.5f - t * 0.5f, h - t, 0.0f},
			{0.0f, h - t, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}, {7, 0}
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
	case ' ':
	default:
		// 空白（スペース）は線なし
		break;
	}
}
