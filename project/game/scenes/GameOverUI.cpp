#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "GameOverUI.h"
#include "WireframeObject.h"
#include "WireframeRenderer.h"
#include "Camera.h"
#include "Sprite.h"
#include "Input.h"
#include "TimeManager.h"
#include "PostProcessRenderer.h"
#include "PostProcessEffects.h"
#include "CrtTransition.h"

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
	inline Vector3 operator*(const Vector3& a, float s) {
		return { a.x * s, a.y * s, a.z * s };
	}
	inline Vector3 operator*(float s, const Vector3& a) {
		return { a.x * s, a.y * s, a.z * s };
	}

	float EaseOutQuad(float t) {
		t = (std::max)(0.0f, (std::min)(1.0f, t));
		return 1.0f - (1.0f - t) * (1.0f - t);
	}
}

GameOverUI::GameOverUI() = default;
GameOverUI::~GameOverUI() = default;

void GameOverUI::Initialize() {
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
	backdropSprite_->SetColor({0.015f, 0.010f, 0.035f, 0.0f});

	// 3. メイン文字・サブ文字・外枠フレームの構築
	BuildMainLetters();
	BuildSubLetters();
	BuildFrame();
}

void GameOverUI::Start() {
	isActive_ = true;
	timer_ = 0.0f;
	backdropAlpha_ = 0.0f;
	isRetryRequested_ = false;
	isTitleRequested_ = false;

	// 自機消滅完了の瞬間に控えめなサイバーパルス
	isGlitching_ = true;
	glitchBurstTimer_ = 0.0f;
	glitchBurstDuration_ = 0.10f;
	glitchIntervalTimer_ = 0.0f;
	glitchNextInterval_ = 0.70f;
	currentGlitchIntensity_ = 0.28f;
	currentChromaticAberration_ = 0.035f;

	// ポストプロセスをグリッチモードへ移行
	PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kGlitch);
	if (auto* glitch = PostProcessRenderer::GetInstance()->GetEffect<GlitchEffect>(PostProcessRenderer::PostProcessMode::kGlitch)) {
		glitch->SetIntensity(currentGlitchIntensity_);
		glitch->SetChromaticAberration(currentChromaticAberration_);
		glitch->SetScanlineIntensity(0.25f);
		glitch->SetSpeed(24.0f);
		glitch->SetFrequency(1.0f);
		glitch->SetBlockCount(35.0f);
		glitch->SetShiftScale(0.5f);
	}
}

void GameOverUI::Reset() {
	isActive_ = false;
	timer_ = 0.0f;
	backdropAlpha_ = 0.0f;
	isRetryRequested_ = false;
	isTitleRequested_ = false;
	if (backdropSprite_) {
		backdropSprite_->SetColor({0.015f, 0.010f, 0.035f, 0.0f});
	}
	if (!CrtTransition::GetInstance()->IsTransitioning()) {
		PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kNormal);
	}
}

void GameOverUI::Update() {
	if (!isActive_) return;

	float dt = TimeManager::GetInstance()->GetDeltaTime();
	timer_ += dt;

	// 1. 暗幕背景のフェードイン（背景を薄暗く沈める: 約0.6秒）
	const float kFadeDuration = 0.60f;
	if (timer_ < kFadeDuration) {
		float progress = timer_ / kFadeDuration;
		backdropAlpha_ = EaseOutQuad(progress) * kTargetBackdropAlpha;
	} else {
		backdropAlpha_ = kTargetBackdropAlpha;
	}
	if (backdropSprite_) {
		backdropSprite_->SetColor({0.012f, 0.008f, 0.030f, backdropAlpha_});
		backdropSprite_->Update();
	}

	// 2. グリッチバーストのリズム・制御（視認性を損なわない上品なノイズ）
	if (isGlitching_) {
		glitchBurstTimer_ += dt;
		// 控えめで程よい歪み
		currentGlitchIntensity_ = 0.20f + (static_cast<float>(rand()) / RAND_MAX) * 0.12f;
		currentChromaticAberration_ = 0.025f + (static_cast<float>(rand()) / RAND_MAX) * 0.015f;

		// バースト中は文字が軽くピクッと揺れる程度
		jitterOffset_.x = ((static_cast<float>(rand()) / RAND_MAX) - 0.5f) * 0.04f;
		jitterOffset_.y = ((static_cast<float>(rand()) / RAND_MAX) - 0.5f) * 0.015f;

		if (glitchBurstTimer_ >= glitchBurstDuration_) {
			isGlitching_ = false;
			glitchBurstTimer_ = 0.0f;
			glitchIntervalTimer_ = 0.0f;
			// 次のバーストまでのインターバル (0.6秒〜1.4秒)
			glitchNextInterval_ = 0.60f + (static_cast<float>(rand()) / RAND_MAX) * 0.80f;
		}
	} else {
		glitchIntervalTimer_ += dt;
		// 定常時は極めて微弱な走査線とクリアな視認性
		currentGlitchIntensity_ = 0.03f;
		currentChromaticAberration_ = 0.008f;
		jitterOffset_ = { 0.0f, 0.0f, 0.0f };

		if (glitchIntervalTimer_ >= glitchNextInterval_) {
			isGlitching_ = true;
			glitchBurstTimer_ = 0.0f;
			// 一瞬だけ（0.04秒〜0.06秒）の軽いパルス
			glitchBurstDuration_ = 0.04f + (static_cast<float>(rand()) / RAND_MAX) * 0.03f;
		}
	}

	// ポストプロセスへパラメータ反映
	if (auto* glitch = PostProcessRenderer::GetInstance()->GetEffect<GlitchEffect>(PostProcessRenderer::PostProcessMode::kGlitch)) {
		glitch->SetIntensity(currentGlitchIntensity_);
		glitch->SetChromaticAberration(currentChromaticAberration_);
		glitch->SetScanlineIntensity(0.20f);
		glitch->SetSpeed(24.0f);
		glitch->SetFrequency(1.0f);
		glitch->SetBlockCount(35.0f);
		glitch->SetShiftScale(isGlitching_ ? 0.6f : 0.3f);
	}

	// 3. メイン文字 "GAME OVER" の出現・色・位置更新（開始から約0.3秒で出現）
	float textAppear = 0.0f;
	if (timer_ >= 0.05f) {
		float t = (timer_ - 0.05f) / 0.35f;
		textAppear = EaseOutQuad(t);
	}

	// 色収差ゴースト（シアン）のオフセット（控えめにしてクリアに）
	Vector3 ghostOffset = jitterOffset_;
	if (isGlitching_) {
		ghostOffset.x += 0.015f;
		ghostOffset.y -= 0.005f;
	} else {
		ghostOffset.x += 0.008f;
		ghostOffset.y -= 0.003f;
	}

	// メインロゴ文字
	Vector4 mainColor = { 1.0f, 0.18f, 0.25f, textAppear * 0.95f };
	Vector4 ghostColor = { 0.10f, 0.90f, 1.0f, textAppear * (isGlitching_ ? 0.45f : 0.20f) };

	for (auto& l : mainLetters_) {
		if (l.wireObject) {
			Vector3 pos = mainPosition_ + jitterOffset_;
			pos.x += l.xOffset * mainScale_.x;
			l.wireObject->SetPosition(pos);
			l.wireObject->SetColor(mainColor);
			l.wireObject->Update();
		}
		if (l.ghostWireObject) {
			Vector3 pos = mainPosition_ + ghostOffset;
			pos.x += l.xOffset * mainScale_.x;
			l.ghostWireObject->SetPosition(pos);
			l.ghostWireObject->SetColor(ghostColor);
			l.ghostWireObject->Update();
		}
	}

	// 外枠フレーム
	if (frameObject_) {
		frameObject_->SetPosition(mainPosition_ + jitterOffset_);
		frameObject_->SetColor({ 1.0f, 0.25f, 0.35f, textAppear * 0.80f });
		frameObject_->Update();
	}
	if (frameGhostObject_) {
		frameGhostObject_->SetPosition(mainPosition_ + ghostOffset);
		frameGhostObject_->SetColor({ 0.10f, 0.85f, 1.0f, textAppear * (isGlitching_ ? 0.35f : 0.15f) });
		frameGhostObject_->Update();
	}

	// 4. サブヘッダー "SYSTEM FAILURE" の更新
	float headerAppear = 0.0f;
	if (timer_ >= 0.25f) {
		float t = (timer_ - 0.25f) / 0.30f;
		headerAppear = EaseOutQuad(t);
	}
	Vector4 headerColor = { 1.0f, 0.40f, 0.45f, headerAppear * 0.85f };
	for (auto& l : headerLetters_) {
		if (l.wireObject) {
			Vector3 pos = headerPosition_ + jitterOffset_ * 0.5f;
			pos.x += l.xOffset * headerScale_.x;
			l.wireObject->SetPosition(pos);
			l.wireObject->SetColor(headerColor);
			l.wireObject->Update();
		}
	}

	// 5. サブプロンプト "ENTER : RETRY    SPACE : TITLE" の点滅更新
	float promptAppear = 0.0f;
	if (timer_ >= 0.50f) {
		float blink = 0.55f + 0.45f * std::sin((timer_ - 0.50f) * 4.0f);
		promptAppear = blink;
	}
	Vector4 promptColor = { 0.90f, 0.95f, 1.0f, promptAppear * 0.90f };
	for (auto& l : promptLetters_) {
		if (l.wireObject) {
			Vector3 pos = promptPosition_;
			pos.x += l.xOffset * promptScale_.x;
			l.wireObject->SetPosition(pos);
			l.wireObject->SetColor(promptColor);
			l.wireObject->Update();
		}
	}

	// 6. 入力判定（操作によるリトライ・タイトル遷移: 遷移中や既に要求済みの場合は無視）
	if (timer_ >= 0.30f && !CrtTransition::GetInstance()->IsTransitioning() && !isRetryRequested_ && !isTitleRequested_) {
		Input* input = Input::GetInstance();
		if (input->TriggerKey(DIK_RETURN) || input->TriggerButton(XINPUT_GAMEPAD_A)) {
			isRetryRequested_ = true;
		}
		if (input->TriggerKey(DIK_SPACE) || input->TriggerKey(DIK_ESCAPE) ||
			input->TriggerButton(XINPUT_GAMEPAD_B) || input->TriggerButton(XINPUT_GAMEPAD_START)) {
			isTitleRequested_ = true;
		}
	}
}

void GameOverUI::Draw() {
	if (!isActive_) return;

	// 1. 暗幕背景（背後の世界を暗く覆う）
	if (backdropSprite_ && backdropAlpha_ > 0.001f) {
		backdropSprite_->Draw();
	}

	// 2. メインロゴ "GAME OVER" 外枠フレーム
	if (frameGhostObject_) {
		frameGhostObject_->Draw();
	}
	if (frameObject_) {
		frameObject_->Draw();
	}

	// 3. メインロゴ文字（シアンゴースト ➔ 赤メイン）
	for (auto& l : mainLetters_) {
		if (l.ghostWireObject) {
			l.ghostWireObject->Draw();
		}
	}
	for (auto& l : mainLetters_) {
		if (l.wireObject) {
			l.wireObject->Draw();
		}
	}

	// 4. サブヘッダー "SYSTEM FAILURE"
	if (timer_ >= 0.25f) {
		for (auto& l : headerLetters_) {
			if (l.wireObject) {
				l.wireObject->Draw();
			}
		}
	}

	// 5. サブプロンプト
	if (timer_ >= 0.50f) {
		for (auto& l : promptLetters_) {
			if (l.wireObject) {
				l.wireObject->Draw();
			}
		}
	}
}

void GameOverUI::BuildMainLetters() {
	mainLetters_.clear();
	const std::string text = "GAME OVER";
	mainLetters_.resize(text.size());

	const float letterH = 1.30f;
	const float letterT = 0.20f;
	const float spacing = 0.18f;

	float totalWidth = 0.0f;
	for (size_t i = 0; i < text.size(); ++i) {
		mainLetters_[i].character = text[i];
		float w = 0.95f;
		if (text[i] == 'M') w = 1.15f;
		else if (text[i] == 'E') w = 0.85f;
		else if (text[i] == 'A') w = 0.95f;
		else if (text[i] == 'R') w = 0.90f;
		else if (text[i] == 'V') w = 0.95f;
		else if (text[i] == ' ') w = 0.48f;

		GenerateCharMesh(text[i], w, letterH, letterT, mainLetters_[i]);
		totalWidth += mainLetters_[i].width;
		if (i + 1 < text.size()) totalWidth += spacing;
	}

	float currentX = -totalWidth * 0.5f;
	const float yCenterOffset = -letterH * 0.5f; // 上下に偏らないよう中心合わせ
	const float halfDepth = mainExtrusionDepth_ * 0.5f;

	for (size_t i = 0; i < mainLetters_.size(); ++i) {
		auto& l = mainLetters_[i];
		l.xOffset = currentX;
		currentX += l.width + spacing;

		if (l.character == ' ') continue;

		// 3D立体押し出し（前面 + 背面 + 奥行き連結梁）
		std::vector<Vector3> frontVerts = l.localVertices;
		std::vector<std::pair<int, int>> frontEdges = l.baseEdges;
		const size_t N = frontVerts.size();

		l.localVertices.clear();
		l.localVertices.reserve(N * 2);

		// 前面頂点（Y座標を中心合わせ）
		for (const auto& v : frontVerts) {
			l.localVertices.push_back({ v.x, v.y + yCenterOffset, -halfDepth });
		}
		// 背面頂点（Y座標を中心合わせ）
		for (const auto& v : frontVerts) {
			l.localVertices.push_back({ v.x, v.y + yCenterOffset, halfDepth });
		}

		l.baseEdges.clear();
		l.baseEdges.reserve(frontEdges.size() * 2 + N);

		// 前面エッジ
		for (const auto& e : frontEdges) {
			l.baseEdges.push_back(e);
		}
		// 背面エッジ
		for (const auto& e : frontEdges) {
			l.baseEdges.push_back({ static_cast<int>(e.first + N), static_cast<int>(e.second + N) });
		}
		// 梁エッジ
		for (size_t v = 0; v < N; ++v) {
			l.baseEdges.push_back({ static_cast<int>(v), static_cast<int>(v + N) });
		}

		// 線分ペアリストに変換
		std::vector<std::pair<Vector3, Vector3>> lines;
		lines.reserve(l.baseEdges.size());
		for (const auto& e : l.baseEdges) {
			lines.push_back({ l.localVertices[e.first], l.localVertices[e.second] });
		}

		// メインワイヤーオブジェクト
		l.wireObject = std::make_unique<WireframeObject>();
		l.wireObject->Initialize();
		l.wireObject->SetCamera(hudCamera_.get());
		l.wireObject->CreateDynamicLineMesh(static_cast<uint32_t>(lines.size() + 8));
		l.wireObject->SetPosition(mainPosition_);
		l.wireObject->SetScale(mainScale_);
		l.wireObject->UpdateDynamicLines(lines);

		// ゴースト（色収差）ワイヤーオブジェクト
		l.ghostWireObject = std::make_unique<WireframeObject>();
		l.ghostWireObject->Initialize();
		l.ghostWireObject->SetCamera(hudCamera_.get());
		l.ghostWireObject->CreateDynamicLineMesh(static_cast<uint32_t>(lines.size() + 8));
		l.ghostWireObject->SetPosition(mainPosition_);
		l.ghostWireObject->SetScale(mainScale_);
		l.ghostWireObject->UpdateDynamicLines(lines);
	}
}

void GameOverUI::BuildSubLetters() {
	// 1. サブヘッダー "SYSTEM FAILURE"
	const std::string headerText = "SYSTEM FAILURE";
	headerLetters_.resize(headerText.size());

	const float hH = 0.90f;
	const float hT = 0.16f;
	const float hSpacing = 0.16f;

	float totalHeaderW = 0.0f;
	for (size_t i = 0; i < headerText.size(); ++i) {
		headerLetters_[i].character = headerText[i];
		float w = 0.85f;
		if (headerText[i] == 'M') w = 1.05f;
		else if (headerText[i] == 'I') w = 0.35f;
		else if (headerText[i] == 'L') w = 0.70f;
		else if (headerText[i] == ' ') w = 0.45f;

		GenerateCharMesh(headerText[i], w, hH, hT, headerLetters_[i]);
		totalHeaderW += headerLetters_[i].width;
		if (i + 1 < headerText.size()) totalHeaderW += hSpacing;
	}

	float curHX = -totalHeaderW * 0.5f;
	const float hYCenterOffset = -hH * 0.5f;
	for (size_t i = 0; i < headerLetters_.size(); ++i) {
		auto& l = headerLetters_[i];
		l.xOffset = curHX;
		curHX += l.width + hSpacing;

		if (l.character == ' ') continue;

		std::vector<std::pair<Vector3, Vector3>> lines;
		for (const auto& e : l.baseEdges) {
			Vector3 p0 = { l.localVertices[e.first].x, l.localVertices[e.first].y + hYCenterOffset, 0.0f };
			Vector3 p1 = { l.localVertices[e.second].x, l.localVertices[e.second].y + hYCenterOffset, 0.0f };
			lines.push_back({ p0, p1 });
		}

		l.wireObject = std::make_unique<WireframeObject>();
		l.wireObject->Initialize();
		l.wireObject->SetCamera(hudCamera_.get());
		l.wireObject->CreateDynamicLineMesh(static_cast<uint32_t>(lines.size() + 4));
		l.wireObject->SetPosition(headerPosition_);
		l.wireObject->SetScale(headerScale_);
		l.wireObject->UpdateDynamicLines(lines);
	}

	// 2. サブプロンプト "ENTER : RETRY    SPACE : TITLE"
	const std::string promptText = "ENTER : RETRY   SPACE : TITLE";
	promptLetters_.resize(promptText.size());

	const float pH = 0.85f;
	const float pT = 0.15f;
	const float pSpacing = 0.14f;

	float totalPromptW = 0.0f;
	for (size_t i = 0; i < promptText.size(); ++i) {
		promptLetters_[i].character = promptText[i];
		float w = 0.80f;
		if (promptText[i] == 'I') w = 0.35f;
		else if (promptText[i] == 'L') w = 0.65f;
		else if (promptText[i] == ':') w = 0.40f;
		else if (promptText[i] == ' ') w = 0.38f;

		GenerateCharMesh(promptText[i], w, pH, pT, promptLetters_[i]);
		totalPromptW += promptLetters_[i].width;
		if (i + 1 < promptText.size()) totalPromptW += pSpacing;
	}

	float curPX = -totalPromptW * 0.5f;
	const float pYCenterOffset = -pH * 0.5f;
	for (size_t i = 0; i < promptLetters_.size(); ++i) {
		auto& l = promptLetters_[i];
		l.xOffset = curPX;
		curPX += l.width + pSpacing;

		if (l.character == ' ') continue;

		std::vector<std::pair<Vector3, Vector3>> lines;
		for (const auto& e : l.baseEdges) {
			Vector3 p0 = { l.localVertices[e.first].x, l.localVertices[e.first].y + pYCenterOffset, 0.0f };
			Vector3 p1 = { l.localVertices[e.second].x, l.localVertices[e.second].y + pYCenterOffset, 0.0f };
			lines.push_back({ p0, p1 });
		}

		l.wireObject = std::make_unique<WireframeObject>();
		l.wireObject->Initialize();
		l.wireObject->SetCamera(hudCamera_.get());
		l.wireObject->CreateDynamicLineMesh(static_cast<uint32_t>(lines.size() + 4));
		l.wireObject->SetPosition(promptPosition_);
		l.wireObject->SetScale(promptScale_);
		l.wireObject->UpdateDynamicLines(lines);
	}
}

void GameOverUI::BuildFrame() {
	const float fw = 5.55f; // 文字の半幅約4.75fに対し、左右に十分な余白
	const float fh = 1.05f; // 文字の上下約±0.65fに対し、上下に心地よい余白
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
		// 上下のサイバー装飾バー
		{{-1.6f,  fh + 0.08f, 0.0f}, { 1.6f,  fh + 0.08f, 0.0f}},
		{{-1.6f, -fh - 0.08f, 0.0f}, { 1.6f, -fh - 0.08f, 0.0f}}
	};

	frameObject_ = std::make_unique<WireframeObject>();
	frameObject_->Initialize();
	frameObject_->SetCamera(hudCamera_.get());
	frameObject_->CreateDynamicLineMesh(static_cast<uint32_t>(frameLines.size() + 4));
	frameObject_->SetPosition(mainPosition_);
	frameObject_->SetScale(mainScale_);
	frameObject_->UpdateDynamicLines(frameLines);

	frameGhostObject_ = std::make_unique<WireframeObject>();
	frameGhostObject_->Initialize();
	frameGhostObject_->SetCamera(hudCamera_.get());
	frameGhostObject_->CreateDynamicLineMesh(static_cast<uint32_t>(frameLines.size() + 4));
	frameGhostObject_->SetPosition(mainPosition_);
	frameGhostObject_->SetScale(mainScale_);
	frameGhostObject_->UpdateDynamicLines(frameLines);
}

void GameOverUI::GenerateCharMesh(char c, float w, float h, float t, LetterData& l) {
	l.width = w;
	l.localVertices.clear();
	l.baseEdges.clear();

	const float cut = t * 1.1f;

	switch (c) {
	case 'G': {
		const float midY = h * 0.45f;
		l.localVertices = {
			// 外周 (0..7)
			{w, h, 0.0f}, {cut, h, 0.0f}, {0.0f, h - cut, 0.0f},
			{0.0f, cut, 0.0f}, {cut, 0.0f, 0.0f}, {w, 0.0f, 0.0f},
			{w, midY, 0.0f}, {w * 0.52f, midY, 0.0f},
			// 内周 (8..15)
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
	case 'M': {
		l.localVertices = {
			{0.0f, 0.0f, 0.0f}, {0.0f, h, 0.0f}, {t * 1.2f, h, 0.0f},
			{w * 0.50f, h * 0.38f, 0.0f},
			{w - t * 1.2f, h, 0.0f}, {w, h, 0.0f}, {w, 0.0f, 0.0f},
			{w - t, 0.0f, 0.0f}, {w - t, h * 0.65f, 0.0f},
			{w * 0.50f, h * 0.15f, 0.0f},
			{t, h * 0.65f, 0.0f}, {t, 0.0f, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6},
			{6, 7}, {7, 8}, {8, 9}, {9, 10}, {10, 11}, {11, 0}
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
	case 'V': {
		l.localVertices = {
			{0.0f, h, 0.0f}, {w * 0.40f, 0.0f, 0.0f}, {w * 0.60f, 0.0f, 0.0f}, {w, h, 0.0f},
			{w - t * 1.1f, h, 0.0f}, {w * 0.50f, t * 1.4f, 0.0f}, {t * 1.1f, h, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 0}
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
	case 'S': {
		const float midY = h * 0.50f;
		const float ht = t * 0.50f;
		l.localVertices = {
			{w, h, 0.0f}, {0.0f, h, 0.0f}, {0.0f, midY - ht, 0.0f},
			{w - t, midY - ht, 0.0f}, {w - t, t, 0.0f}, {0.0f, t, 0.0f},
			{0.0f, 0.0f, 0.0f}, {w, 0.0f, 0.0f}, {w, midY + ht, 0.0f},
			{t, midY + ht, 0.0f}, {t, h - t, 0.0f}, {w, h - t, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6},
			{6, 7}, {7, 8}, {8, 9}, {9, 10}, {10, 11}, {11, 0}
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
	case 'N': {
		l.localVertices = {
			{0.0f, 0.0f, 0.0f}, {0.0f, h, 0.0f}, {t * 1.2f, h, 0.0f},
			{w - t, t * 1.4f, 0.0f}, {w - t, h, 0.0f}, {w, h, 0.0f},
			{w, 0.0f, 0.0f}, {w - t * 1.2f, 0.0f, 0.0f},
			{t, h - t * 1.4f, 0.0f}, {t, 0.0f, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}, {7, 8}, {8, 9}, {9, 0}
		};
		break;
	}
	case 'F': {
		l.localVertices = {
			{0.0f, 0.0f, 0.0f}, {0.0f, h, 0.0f}, {w, h, 0.0f}, {w, h - t, 0.0f},
			{t, h - t, 0.0f}, {t, h * 0.58f, 0.0f}, {w * 0.75f, h * 0.58f, 0.0f},
			{w * 0.75f, h * 0.42f, 0.0f}, {t, h * 0.42f, 0.0f}, {t, 0.0f, 0.0f}
		};
		l.baseEdges = {
			{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}, {7, 8}, {8, 9}, {9, 0}
		};
		break;
	}
	case 'U': {
		l.localVertices = {
			{0.0f, h, 0.0f}, {0.0f, cut, 0.0f}, {cut, 0.0f, 0.0f},
			{w - cut, 0.0f, 0.0f}, {w, cut, 0.0f}, {w, h, 0.0f},
			{w - t, h, 0.0f}, {w - t, cut + t * 0.2f, 0.0f},
			{w - cut - t * 0.2f, t, 0.0f}, {cut + t * 0.2f, t, 0.0f},
			{t, cut + t * 0.2f, 0.0f}, {t, h, 0.0f}
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
