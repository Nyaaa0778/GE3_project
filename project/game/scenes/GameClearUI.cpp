#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "GameClearUI.h"
#include "WireframeObject.h"
#include "WireframeRenderer.h"
#include "Camera.h"
#include "Sprite.h"
#include "Input.h"
#include "TimeManager.h"
#include "MathUtility.h"

#include <cmath>
#include <algorithm>
#include <cstdlib>

namespace {
	inline Vector3 operator+(const Vector3& a, const Vector3& b) {
		return { a.x + b.x, a.y + b.y, a.z + b.z };
	}
	inline Vector3 operator-(const Vector3& a, const Vector3& b) {
		return { a.x - b.x, a.y - b.y, a.z - b.z };
	}

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

GameClearUI::GameClearUI() = default;
GameClearUI::~GameClearUI() = default;

void GameClearUI::Initialize() {
	// 1. 専用HUD正面カメラ（画角・位置を固定して画面中央へ正確に配置）
	hudCamera_ = std::make_unique<Camera>();
	hudCamera_->SetFarClip(100.0f);
	hudCamera_->CreateConstantBuffer();
	hudCamera_->SetTranslate({0.0f, 0.0f, -11.0f});
	hudCamera_->SetRotate({0.0f, 0.0f, 0.0f});
	hudCamera_->CalculateMatrix();

	// 2. 全画面暗幕背景スプライト（背後の3D世界を薄暗く落とし込む）
	backdropSprite_ = std::make_unique<Sprite>();
	backdropSprite_->Initialize("white.png", {0.0f, 0.0f}, {0.0f, 0.0f});
	backdropSprite_->SetSize({1280.0f, 720.0f});
	backdropSprite_->SetColor({0.010f, 0.018f, 0.035f, 0.0f});

	// 3. 数字スプライト用スリムプレート（控えめなHUDトレイ）
	scorePlateSprite_ = std::make_unique<Sprite>();
	scorePlateSprite_->Initialize("white.png", {645.0f, 348.0f}, {0.0f, 0.5f});
	scorePlateSprite_->SetSize({155.0f, 34.0f});
	scorePlateSprite_->SetColor({0.015f, 0.045f, 0.090f, 0.0f});

	killPlateSprite_ = std::make_unique<Sprite>();
	killPlateSprite_->Initialize("white.png", {645.0f, 420.0f}, {0.0f, 0.5f});
	killPlateSprite_->SetSize({155.0f, 34.0f});
	killPlateSprite_->SetColor({0.015f, 0.045f, 0.090f, 0.0f});

	// 5. 数字スプライトの生成 (小さく洗練されたコンパクトサイズ: 幅18px, 高さ26px)
	const float digitW = 18.0f;
	const float digitH = 26.0f;
	const float digitSpacing = 22.0f;

	// スコア数字 (6桁、右端 785px 付近)
	scoreDigits_.resize(kScoreDigits);
	const float scoreRightX = 785.0f;
	const float scoreY = 335.0f;
	for (int i = 0; i < kScoreDigits; ++i) {
		scoreDigits_[i] = std::make_unique<Sprite>();
		Vector2 pos = { scoreRightX - (i + 1) * digitSpacing, scoreY };
		scoreDigits_[i]->Initialize("numbers/0.png", pos, {0.0f, 0.0f});
		scoreDigits_[i]->SetSize({digitW, digitH});
		scoreDigits_[i]->SetBlendMode(BlendMode::kAdd);
		scoreDigits_[i]->SetColor({0.0f, 0.95f, 1.0f, 0.0f});
	}

	// 撃破数数字 (3桁、右端 785px 付近)
	killDigits_.resize(kKillDigits);
	const float killRightX = 785.0f;
	const float killY = 407.0f;
	for (int i = 0; i < kKillDigits; ++i) {
		killDigits_[i] = std::make_unique<Sprite>();
		Vector2 pos = { killRightX - (i + 1) * digitSpacing, killY };
		killDigits_[i]->Initialize("numbers/0.png", pos, {0.0f, 0.0f});
		killDigits_[i]->SetSize({digitW, digitH});
		killDigits_[i]->SetBlendMode(BlendMode::kAdd);
		killDigits_[i]->SetColor({1.0f, 0.75f, 0.2f, 0.0f});
	}

	// 6. 3D立体タイトル「STAGE CLEAR」の構築
	Build3DTitleLetters();

	// 7. 小さく端正なサブラベル構築 (高さ 0.20f, 太さ 0.035f)
	BuildSubTextLetters("SCORE", scoreLabelLetters_, 0.20f, 0.035f, 0.045f, scoreLabelPosition_);
	BuildSubTextLetters("TARGETS", targetLabelLetters_, 0.20f, 0.035f, 0.045f, targetLabelPosition_);
	BuildSubTextLetters("SPACE : TITLE", promptLetters_, 0.16f, 0.028f, 0.040f, promptPosition_);

	// 8. 外枠フレーム
	BuildFrame();
}

void GameClearUI::Build3DTitleLetters() {
	titleLetters_.clear();
	const std::string text = "STAGE CLEAR";
	titleLetters_.resize(text.size());

	const float letterH = 1.15f;
	const float letterT = 0.18f;
	const float spacing = 0.14f;
	const float halfDepth = extrusionDepth_ * 0.5f;
	const float yCenterOffset = -letterH * 0.5f;
	const float frontLineThickness = 0.020f;

	float totalWidth = 0.0f;
	for (size_t i = 0; i < text.size(); ++i) {
		char c = text[i];
		titleLetters_[i].character = c;
		float w = 0.90f;
		if (c == ' ') w = 0.45f;
		else if (c == 'I') w = 0.35f;
		else if (c == 'T' || c == 'L') w = 0.80f;

		GenerateCharMesh(c, w, letterH, letterT, titleLetters_[i]);
		totalWidth += titleLetters_[i].width;
		if (i + 1 < text.size()) totalWidth += spacing;
	}

	float currentX = -totalWidth * 0.5f;

	for (size_t i = 0; i < titleLetters_.size(); ++i) {
		auto& l = titleLetters_[i];
		l.xOffset = currentX + l.width * 0.5f;
		currentX += l.width + spacing;

		if (l.localVertices.empty() || l.baseEdges.empty()) {
			continue;
		}

		// 3D立体押し出し（前面 + 背面 + 奥行き連結梁）
		std::vector<Vector3> frontVerts = l.localVertices;
		std::vector<std::pair<int, int>> frontEdges = l.baseEdges;
		const size_t N = frontVerts.size();

		l.localVertices.clear();
		l.localVertices.reserve(N * 2);

		// 1. 前面頂点 (Z = -halfDepth)
		for (const auto& v : frontVerts) {
			l.localVertices.push_back({v.x - l.width * 0.5f, v.y + yCenterOffset, -halfDepth});
		}
		// 2. 背面頂点 (Z = +halfDepth)
		for (const auto& v : frontVerts) {
			l.localVertices.push_back({v.x - l.width * 0.5f, v.y + yCenterOffset, halfDepth});
		}

		std::vector<Vector4> finalVertices;
		std::vector<uint32_t> finalIndices;

		auto addLine = [&](const Vector3& p0, const Vector3& p1) {
			uint32_t baseIdx = static_cast<uint32_t>(finalVertices.size());
			finalVertices.push_back({p0.x, p0.y, p0.z, 1.0f});
			finalVertices.push_back({p1.x, p1.y, p1.z, 1.0f});
			finalIndices.push_back(baseIdx);
			finalIndices.push_back(baseIdx + 1);
		};

		// 前面エッジ（太いダブルライン）
		for (const auto& e : frontEdges) {
			const Vector3& p0 = l.localVertices[e.first];
			const Vector3& p1 = l.localVertices[e.second];
			Vector3 n = CalculateNormalOffset(p0, p1, frontLineThickness);
			addLine(p0 + n, p1 + n);
			addLine(p0 - n, p1 - n);
		}

		// 背面エッジ（単線）
		for (const auto& e : frontEdges) {
			const Vector3& p0 = l.localVertices[e.first + N];
			const Vector3& p1 = l.localVertices[e.second + N];
			addLine(p0, p1);
		}

		// 奥行き連結梁 (Struts)
		for (size_t k = 0; k < N; ++k) {
			const Vector3& p0 = l.localVertices[k];
			const Vector3& p1 = l.localVertices[k + N];
			addLine(p0, p1);
		}

		l.wireObject = std::make_unique<WireframeObject>();
		l.wireObject->Initialize();
		l.wireObject->SetCamera(hudCamera_.get());
		l.wireObject->SetDepthEnable(false);
		l.wireObject->SetBlendMode(BlendMode::kAdd);
		l.wireObject->CreateCustomMesh(finalVertices, finalIndices);

		Vector3 pos = titlePosition_;
		pos.x += l.xOffset * titleScale_.x;
		l.wireObject->SetPosition(pos);
		l.wireObject->SetScale(titleScale_);
		l.wireObject->SetRotation(titleRotation_);
	}
}

// 小さく端正なサブラベルの構築（最初から小さな寸法で頂点を生成し、スケール1.0で正確に配置）
void GameClearUI::BuildSubTextLetters(const std::string& text, std::vector<LetterData>& letterList, float h, float t, float tracking, const Vector3& basePos) {
	letterList.clear();
	letterList.resize(text.size());

	float totalWidth = 0.0f;
	for (size_t i = 0; i < text.size(); ++i) {
		char c = text[i];
		letterList[i].character = c;
		float w = h * 0.72f;
		if (c == ' ') w = h * 0.40f;
		else if (c == 'I') w = h * 0.32f;
		else if (c == ':') w = h * 0.30f;
		else if (c == 'T' || c == 'L') w = h * 0.65f;

		GenerateCharMesh(c, w, h, t, letterList[i]);
		totalWidth += letterList[i].width;
		if (i + 1 < text.size()) totalWidth += tracking;
	}

	float currentX = -totalWidth * 0.5f;
	const float yCenterOffset = -h * 0.5f;

	for (size_t i = 0; i < letterList.size(); ++i) {
		auto& l = letterList[i];
		l.xOffset = currentX + l.width * 0.5f;
		currentX += l.width + tracking;

		if (l.localVertices.empty() || l.baseEdges.empty()) {
			continue;
		}

		std::vector<Vector4> v;
		std::vector<uint32_t> idx;
		for (const auto& pt : l.localVertices) {
			v.push_back({pt.x - l.width * 0.5f, pt.y + yCenterOffset, pt.z, 1.0f});
		}
		for (const auto& edge : l.baseEdges) {
			idx.push_back(edge.first);
			idx.push_back(edge.second);
		}

		l.wireObject = std::make_unique<WireframeObject>();
		l.wireObject->Initialize();
		l.wireObject->SetCamera(hudCamera_.get());
		l.wireObject->SetDepthEnable(false);
		l.wireObject->SetBlendMode(BlendMode::kAdd);
		l.wireObject->CreateCustomMesh(v, idx);

		Vector3 pos = basePos;
		pos.x += l.xOffset;
		l.wireObject->SetPosition(pos);
		l.wireObject->SetScale({1.0f, 1.0f, 1.0f}); // スケール1.0で正確な比率を保持
	}
}

void GameClearUI::Start(int finalScore, int killedCount) {
	isActive_ = true;
	timer_ = 0.0f;
	backdropAlpha_ = 0.0f;
	targetScore_ = finalScore;
	targetKills_ = killedCount;
	displayScore_ = 0;
	displayKills_ = 0;
	isCountUpFinished_ = false;
	isTitleRequested_ = false;
}

void GameClearUI::Update() {
	if (!isActive_) {
		return;
	}

	auto* input = Input::GetInstance();
	float dt = TimeManager::GetInstance()->GetDeltaTime();
	timer_ += dt;

	// 1. 全面背景暗幕フェードイン (約0.4秒)
	if (timer_ < 0.4f) {
		backdropAlpha_ = kTargetBackdropAlpha * EaseOutQuad(timer_ / 0.4f);
	} else {
		backdropAlpha_ = kTargetBackdropAlpha;
	}

	if (backdropSprite_) {
		backdropSprite_->SetColor({0.010f, 0.018f, 0.035f, backdropAlpha_});
		backdropSprite_->Update();
	}

	float titleAppear = EaseOutQuad((std::max)(0.0f, (timer_ - 0.06f) / 0.30f));

	// 2. スコア＆撃破数のカウントアップ
	if (timer_ >= 0.30f) {
		float progress = (timer_ - 0.30f) / 0.85f;
		if (progress >= 1.0f) {
			displayScore_ = targetScore_;
			displayKills_ = targetKills_;
			isCountUpFinished_ = true;
		} else {
			float easeP = EaseOutQuad(progress);
			displayScore_ = static_cast<int>(targetScore_ * easeP);
			displayKills_ = static_cast<int>(targetKills_ * easeP);
		}
	}

	// 4. 入力処理（SPACE / ENTER / Aボタン）
	if (input->TriggerKey(DIK_SPACE) || input->TriggerKey(DIK_RETURN) || input->TriggerButton(XINPUT_GAMEPAD_A)) {
		if (!isCountUpFinished_) {
			displayScore_ = targetScore_;
			displayKills_ = targetKills_;
			isCountUpFinished_ = true;
		} else {
			isTitleRequested_ = true;
		}
	}

	// 5. 3D立体タイトル文字「STAGE CLEAR」の更新
	Vector4 titleColor = {0.10f, 0.95f, 1.0f, titleAppear * 1.0f};

	for (auto& l : titleLetters_) {
		if (l.wireObject) {
			Vector3 pos = titlePosition_;
			pos.x += l.xOffset * titleScale_.x;
			l.wireObject->SetPosition(pos);
			l.wireObject->SetScale(titleScale_);
			l.wireObject->SetRotation(titleRotation_);
			l.wireObject->SetColor(titleColor);
			l.wireObject->Update();
		}
	}

	// 外枠サイバーフレーム
	if (frameObject_) {
		frameObject_->SetPosition({0.0f, 0.0f, 0.0f});
		frameObject_->SetColor({0.0f, 0.75f, 1.0f, titleAppear * 0.60f});
		frameObject_->Update();
	}

	// 6. サブラベル "SCORE" & "TARGETS" の更新（端正な白シアン・ゴールド）
	float labelAppear = EaseOutQuad((std::max)(0.0f, (timer_ - 0.18f) / 0.25f));
	for (auto& l : scoreLabelLetters_) {
		if (l.wireObject) {
			Vector3 pos = scoreLabelPosition_;
			pos.x += l.xOffset;
			l.wireObject->SetPosition(pos);
			l.wireObject->SetScale({1.0f, 1.0f, 1.0f});
			l.wireObject->SetColor({0.70f, 0.92f, 1.0f, labelAppear * 0.85f});
			l.wireObject->Update();
		}
	}

	for (auto& l : targetLabelLetters_) {
		if (l.wireObject) {
			Vector3 pos = targetLabelPosition_;
			pos.x += l.xOffset;
			l.wireObject->SetPosition(pos);
			l.wireObject->SetScale({1.0f, 1.0f, 1.0f});
			l.wireObject->SetColor({1.0f, 0.80f, 0.40f, labelAppear * 0.85f});
			l.wireObject->Update();
		}
	}

	// 7. プロンプト "SPACE : TITLE" の点滅更新（小さく控えめ）
	float promptAlpha = 0.0f;
	if (timer_ >= 0.60f) {
		float blink = 0.4f + 0.5f * std::sin((timer_ - 0.60f) * 4.0f);
		promptAlpha = blink;
	}
	for (auto& l : promptLetters_) {
		if (l.wireObject) {
			Vector3 pos = promptPosition_;
			pos.x += l.xOffset;
			l.wireObject->SetPosition(pos);
			l.wireObject->SetScale({1.0f, 1.0f, 1.0f});
			l.wireObject->SetColor({0.85f, 0.92f, 1.0f, promptAlpha * 0.80f});
			l.wireObject->Update();
		}
	}

	// 8. 数字スプライト＆プレートの更新（コンパクトサイズ）
	float digitsAlpha = EaseOutQuad((std::max)(0.0f, (timer_ - 0.22f) / 0.25f));

	if (scorePlateSprite_) {
		scorePlateSprite_->SetColor({0.015f, 0.045f, 0.090f, digitsAlpha * 0.65f});
		scorePlateSprite_->Update();
	}
	if (killPlateSprite_) {
		killPlateSprite_->SetColor({0.015f, 0.045f, 0.090f, digitsAlpha * 0.65f});
		killPlateSprite_->Update();
	}

	UpdateDigitSprites(scoreDigits_, displayScore_, digitsAlpha, {0.0f, 0.95f, 1.0f, 1.0f});
	UpdateDigitSprites(killDigits_, displayKills_, digitsAlpha, {1.0f, 0.75f, 0.2f, 1.0f});
}

void GameClearUI::UpdateDigitSprites(std::vector<std::unique_ptr<Sprite>>& sprites, int value, float alpha, const Vector4& baseColor) {
	int temp = value;
	for (size_t i = 0; i < sprites.size(); ++i) {
		int digit = temp % 10;
		temp /= 10;
		std::string path = "numbers/" + std::to_string(digit) + ".png";
		sprites[i]->SetTexture(path);
		sprites[i]->SetColor({baseColor.x, baseColor.y, baseColor.z, baseColor.w * alpha});
		sprites[i]->Update();
	}
}

void GameClearUI::Draw3D() {
	if (!isActive_ || backdropAlpha_ <= 0.001f) {
		return;
	}

	// 外枠フレーム
	if (frameObject_) {
		frameObject_->Draw();
	}

	// 3D立体タイトルロゴ
	for (auto& l : titleLetters_) {
		if (l.wireObject) {
			l.wireObject->Draw();
		}
	}

	// サブラベル
	for (auto& l : scoreLabelLetters_) {
		if (l.wireObject) {
			l.wireObject->Draw();
		}
	}
	for (auto& l : targetLabelLetters_) {
		if (l.wireObject) {
			l.wireObject->Draw();
		}
	}

	// プロンプト
	for (auto& l : promptLetters_) {
		if (l.wireObject) {
			l.wireObject->Draw();
		}
	}
}

void GameClearUI::Draw2D() {
	if (!isActive_ || backdropAlpha_ <= 0.001f) {
		return;
	}

	// 全面背景暗幕
	if (backdropSprite_) {
		backdropSprite_->Draw();
	}

	// スロット枠背景
	if (scorePlateSprite_) {
		scorePlateSprite_->Draw();
	}
	if (killPlateSprite_) {
		killPlateSprite_->Draw();
	}

	// ネオン数字スプライト
	for (auto& s : scoreDigits_) {
		if (s) s->Draw();
	}
	for (auto& s : killDigits_) {
		if (s) s->Draw();
	}
}

// 外枠サイバーフレームの構築
void GameClearUI::BuildFrame() {
	frameObject_ = std::make_unique<WireframeObject>();
	frameObject_->Initialize();
	frameObject_->SetCamera(hudCamera_.get());
	frameObject_->SetDepthEnable(false);
	frameObject_->SetBlendMode(BlendMode::kAdd);

	const float hw = 3.2f;
	const float hh = 1.85f;
	const float corner = 0.35f;

	std::vector<Vector4> v = {
		// 外枠
		{-hw + corner, hh, 0.0f, 1.0f}, {hw - corner, hh, 0.0f, 1.0f},
		{-hw + corner, -hh, 0.0f, 1.0f}, {hw - corner, -hh, 0.0f, 1.0f},
		{-hw, hh - corner, 0.0f, 1.0f}, {-hw, -hh + corner, 0.0f, 1.0f},
		{hw, hh - corner, 0.0f, 1.0f}, {hw, -hh + corner, 0.0f, 1.0f},
		// 面取り
		{-hw, hh - corner, 0.0f, 1.0f}, {-hw + corner, hh, 0.0f, 1.0f},
		{hw - corner, hh, 0.0f, 1.0f}, {hw, hh - corner, 0.0f, 1.0f},
		{hw, -hh + corner, 0.0f, 1.0f}, {hw - corner, -hh, 0.0f, 1.0f},
		{-hw + corner, -hh, 0.0f, 1.0f}, {-hw, -hh + corner, 0.0f, 1.0f}
	};

	std::vector<uint32_t> idx;
	for (uint32_t i = 0; i < v.size(); ++i) {
		idx.push_back(i);
	}
	frameObject_->CreateCustomMesh(v, idx);
}

// 洗練サイバーブロックフォント生成関数
void GameClearUI::GenerateCharMesh(char c, float w, float h, float t, LetterData& l) {
	l.width = w;
	l.localVertices.clear();
	l.baseEdges.clear();

	const float cut = t * 1.1f;

	switch (c) {
	case 'S': {
		const float midY = h * 0.50f;
		const float ht = t * 0.50f;
		l.localVertices = {
			{w, h, 0.0f}, {0.0f, h, 0.0f}, {0.0f, midY - ht, 0.0f}, {w - t, midY - ht, 0.0f},
			{w - t, t, 0.0f}, {0.0f, t, 0.0f}, {0.0f, 0.0f, 0.0f}, {w, 0.0f, 0.0f},
			{w, midY + ht, 0.0f}, {t, midY + ht, 0.0f}, {t, h - t, 0.0f}, {w, h - t, 0.0f}
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
	case 'G': {
		const float midY = h * 0.45f;
		l.localVertices = {
			{w, h, 0.0f}, {cut, h, 0.0f}, {0.0f, h - cut, 0.0f},
			{0.0f, cut, 0.0f}, {cut, 0.0f, 0.0f}, {w, 0.0f, 0.0f},
			{w, midY, 0.0f}, {w * 0.52f, midY, 0.0f},
			// 内周
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
	case 'C': {
		l.localVertices = {
			{w, h, 0.0f}, {cut, h, 0.0f}, {0.0f, h - cut, 0.0f},
			{0.0f, cut, 0.0f}, {cut, 0.0f, 0.0f}, {w, 0.0f, 0.0f},
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
	case ':': {
		const float d = t * 0.9f;
		const float cX = w * 0.5f - d * 0.5f;
		l.localVertices = {
			// 上点
			{cX, h * 0.65f, 0.0f}, {cX + d, h * 0.65f, 0.0f},
			{cX + d, h * 0.65f + d, 0.0f}, {cX, h * 0.65f + d, 0.0f},
			// 下点
			{cX, h * 0.25f, 0.0f}, {cX + d, h * 0.25f, 0.0f},
			{cX + d, h * 0.25f + d, 0.0f}, {cX, h * 0.25f + d, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 0},
			{4, 5}, {5, 6}, {6, 7}, {7, 4}
		};
		break;
	}
	case ' ':
	default:
		break;
	}
}
