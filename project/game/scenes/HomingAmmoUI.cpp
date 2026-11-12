#include "HomingAmmoUI.h"
#include "Sprite.h"
#include "Player.h"
#include <algorithm>
#include <cmath>

// ============================================================
// PixelText 実装
// ============================================================
PixelText::PixelText() = default;
PixelText::~PixelText() = default;

void PixelText::Initialize(const std::string& text, const Vector2& pos, float scale) {
	basePos_ = pos;
	scale_ = scale;
	isVisible_ = true;
	segments_.clear();

	const float charW = 8.0f;
	const float charH = 12.0f;
	const float charGap = 3.0f;
	const float t = 2.0f; // 線幅

	float cursorX = 0.0f;

	auto AddSeg = [&](float x, float y, float w, float h) {
		Segment seg;
		seg.offset = { x, y };
		seg.size = { w * scale_, h * scale_ };
		seg.sprite = std::make_unique<Sprite>();
		seg.sprite->Initialize("white.png", { basePos_.x + x * scale_, basePos_.y + y * scale_ }, { 0.0f, 0.0f });
		seg.sprite->SetSize(seg.size);
		segments_.push_back(std::move(seg));
	};

	for (char c : text) {
		const float ox = cursorX;
		switch (c) {
		case 'N':
			AddSeg(ox, 0.0f, t, charH);
			AddSeg(ox + 6.0f, 0.0f, t, charH);
			AddSeg(ox + 2.0f, 3.0f, 2.0f, 3.0f);
			AddSeg(ox + 4.0f, 6.0f, 2.0f, 3.0f);
			break;
		case 'O':
		case '0':
			AddSeg(ox, 0.0f, t, charH);
			AddSeg(ox + 6.0f, 0.0f, t, charH);
			AddSeg(ox + 2.0f, 0.0f, 4.0f, t);
			AddSeg(ox + 2.0f, 10.0f, 4.0f, t);
			break;
		case 'R':
			AddSeg(ox, 0.0f, t, charH);
			AddSeg(ox + 2.0f, 0.0f, 4.0f, t);
			AddSeg(ox + 2.0f, 5.0f, 4.0f, t);
			AddSeg(ox + 6.0f, 0.0f, t, 6.0f);
			AddSeg(ox + 4.0f, 6.0f, 2.0f, 3.0f);
			AddSeg(ox + 6.0f, 9.0f, 2.0f, 3.0f);
			break;
		case 'M':
			AddSeg(ox, 0.0f, t, charH);
			AddSeg(ox + 6.0f, 0.0f, t, charH);
			AddSeg(ox + 2.0f, 2.0f, 2.0f, 3.0f);
			AddSeg(ox + 4.0f, 2.0f, 2.0f, 3.0f);
			AddSeg(ox + 3.0f, 4.0f, 2.0f, 3.0f);
			break;
		case 'A':
			AddSeg(ox, 2.0f, t, 10.0f);
			AddSeg(ox + 6.0f, 2.0f, t, 10.0f);
			AddSeg(ox + 2.0f, 0.0f, 4.0f, t);
			AddSeg(ox + 2.0f, 6.0f, 4.0f, t);
			break;
		case 'L':
			AddSeg(ox, 0.0f, t, charH);
			AddSeg(ox + 2.0f, 10.0f, 6.0f, t);
			break;
		case 'H':
			AddSeg(ox, 0.0f, t, charH);
			AddSeg(ox + 6.0f, 0.0f, t, charH);
			AddSeg(ox + 2.0f, 5.0f, 4.0f, t);
			break;
		case 'I':
		case '1':
			AddSeg(ox + 1.0f, 0.0f, 6.0f, t);
			AddSeg(ox + 1.0f, 10.0f, 6.0f, t);
			AddSeg(ox + 3.0f, 2.0f, t, 8.0f);
			break;
		case 'G':
			AddSeg(ox, 0.0f, t, charH);
			AddSeg(ox + 2.0f, 0.0f, 5.0f, t);
			AddSeg(ox + 2.0f, 10.0f, 5.0f, t);
			AddSeg(ox + 5.0f, 5.0f, t, 6.0f);
			AddSeg(ox + 3.0f, 5.0f, 2.0f, t);
			break;
		case 'E':
			AddSeg(ox, 0.0f, t, charH);
			AddSeg(ox + 2.0f, 0.0f, 5.0f, t);
			AddSeg(ox + 2.0f, 5.0f, 4.0f, t);
			AddSeg(ox + 2.0f, 10.0f, 5.0f, t);
			break;
		case 'W':
			AddSeg(ox, 0.0f, t, charH);
			AddSeg(ox + 6.0f, 0.0f, t, charH);
			AddSeg(ox + 2.0f, 7.0f, 2.0f, 3.0f);
			AddSeg(ox + 4.0f, 7.0f, 2.0f, 3.0f);
			AddSeg(ox + 3.0f, 5.0f, 2.0f, 3.0f);
			break;
		case 'P':
			AddSeg(ox, 0.0f, t, charH);
			AddSeg(ox + 2.0f, 0.0f, 4.0f, t);
			AddSeg(ox + 2.0f, 5.0f, 4.0f, t);
			AddSeg(ox + 6.0f, 0.0f, t, 6.0f);
			break;
		case 'S':
			AddSeg(ox, 0.0f, 7.0f, t);
			AddSeg(ox, 2.0f, t, 4.0f);
			AddSeg(ox + 1.0f, 5.0f, 5.0f, t);
			AddSeg(ox + 5.0f, 6.0f, t, 4.0f);
			AddSeg(ox, 10.0f, 7.0f, t);
			break;
		case 'T':
			AddSeg(ox, 0.0f, 8.0f, t);
			AddSeg(ox + 3.0f, 2.0f, t, 10.0f);
			break;
		case 'C':
			AddSeg(ox, 0.0f, t, charH);
			AddSeg(ox + 2.0f, 0.0f, 5.0f, t);
			AddSeg(ox + 2.0f, 10.0f, 5.0f, t);
			break;
		case 'D':
			AddSeg(ox, 0.0f, t, charH);
			AddSeg(ox + 2.0f, 0.0f, 3.0f, t);
			AddSeg(ox + 2.0f, 10.0f, 3.0f, t);
			AddSeg(ox + 5.0f, 2.0f, t, 8.0f);
			break;
		case '[':
			AddSeg(ox + 2.0f, 0.0f, t, charH);
			AddSeg(ox + 2.0f, 0.0f, 4.0f, t);
			AddSeg(ox + 2.0f, 10.0f, 4.0f, t);
			break;
		case ']':
			AddSeg(ox + 2.0f, 0.0f, t, charH);
			AddSeg(ox, 0.0f, 4.0f, t);
			AddSeg(ox, 10.0f, 4.0f, t);
			break;
		case '-':
			AddSeg(ox + 1.0f, 5.0f, 5.0f, t);
			break;
		case '>':
			AddSeg(ox + 1.0f, 1.0f, 2.0f, 3.0f);
			AddSeg(ox + 3.0f, 4.0f, 2.0f, 3.0f);
			AddSeg(ox + 5.0f, 5.0f, 2.0f, 2.0f);
			AddSeg(ox + 3.0f, 7.0f, 2.0f, 3.0f);
			AddSeg(ox + 1.0f, 9.0f, 2.0f, 3.0f);
			break;
		case '<':
			AddSeg(ox + 5.0f, 1.0f, 2.0f, 3.0f);
			AddSeg(ox + 3.0f, 4.0f, 2.0f, 3.0f);
			AddSeg(ox + 1.0f, 5.0f, 2.0f, 2.0f);
			AddSeg(ox + 3.0f, 7.0f, 2.0f, 3.0f);
			AddSeg(ox + 5.0f, 9.0f, 2.0f, 3.0f);
			break;
		case ' ':
		default:
			break;
		}
		cursorX += (charW + charGap);
	}
	totalWidth_ = cursorX * scale_;
}

void PixelText::SetPosition(const Vector2& pos) {
	basePos_ = pos;
	for (auto& seg : segments_) {
		seg.sprite->SetPosition({ basePos_.x + seg.offset.x * scale_, basePos_.y + seg.offset.y * scale_ });
	}
}

void PixelText::SetColor(const Vector4& color) {
	for (auto& seg : segments_) {
		seg.sprite->SetColor(color);
	}
}

void PixelText::Update() {
	if (!isVisible_) return;
	for (auto& seg : segments_) {
		seg.sprite->Update();
	}
}

void PixelText::Draw() {
	if (!isVisible_) return;
	for (auto& seg : segments_) {
		seg.sprite->Draw();
	}
}

// ============================================================
// HomingAmmoUI 実装
// ============================================================
HomingAmmoUI::HomingAmmoUI() = default;
HomingAmmoUI::~HomingAmmoUI() = default;

void HomingAmmoUI::Initialize(const Vector2& basePos) {
	basePos_ = basePos;
	isVisible_ = true;
	prevAmmo_ = -1;
	prevSec_ = -1;
	prevTenth_ = -1;
	pulseTimer_ = 0.0f;
	isFull_ = true;
	isLockOnMode_ = false;
	keyFlashTimer_ = 0.0f;
	switchBannerTimer_ = 0.0f;

	// ------------------------------------------------------------
	// 1. 上段：武器モードセレクターバー
	// ------------------------------------------------------------
	// 通常弾タブ
	normalTabBgSprite_ = std::make_unique<Sprite>();
	normalTabBgSprite_->Initialize("white.png", basePos_, { 0.0f, 0.0f });
	normalTabBgSprite_->SetSize({ 116.0f, 19.0f });
	normalTabBgSprite_->SetColor({ 0.04f, 0.22f, 0.30f, 0.85f });

	normalTabBorderSprite_ = std::make_unique<Sprite>();
	normalTabBorderSprite_->Initialize("white.png", basePos_, { 0.0f, 0.0f });
	normalTabBorderSprite_->SetSize({ 116.0f, 1.0f });
	normalTabBorderSprite_->SetColor({ 0.0f, 0.85f, 1.0f, 0.50f });

	normalTabActiveLineSprite_ = std::make_unique<Sprite>();
	normalTabActiveLineSprite_->Initialize("white.png", { basePos_.x, basePos_.y + 17.0f }, { 0.0f, 0.0f });
	normalTabActiveLineSprite_->SetSize({ 116.0f, 2.0f });
	normalTabActiveLineSprite_->SetColor({ 0.0f, 0.95f, 1.0f, 1.0f });

	// 通常弾アイコン：前方直進矢印ライン [──►]
	normalTabIconLineSprite_ = std::make_unique<Sprite>();
	normalTabIconLineSprite_->Initialize("white.png", { basePos_.x + 8.0f, basePos_.y + 9.0f }, { 0.0f, 0.0f });
	normalTabIconLineSprite_->SetSize({ 12.0f, 2.0f });
	normalTabIconLineSprite_->SetColor({ 0.2f, 0.95f, 1.0f, 1.0f });

	normalTabIconHeadSprite_ = std::make_unique<Sprite>();
	normalTabIconHeadSprite_->Initialize("white.png", { basePos_.x + 18.0f, basePos_.y + 7.0f }, { 0.0f, 0.0f });
	normalTabIconHeadSprite_->SetSize({ 3.0f, 6.0f });
	normalTabIconHeadSprite_->SetColor({ 0.2f, 0.95f, 1.0f, 1.0f });

	normalText_ = std::make_unique<PixelText>();
	normalText_->Initialize("NORMAL", { basePos_.x + 28.0f, basePos_.y + 4.0f }, 0.85f);
	normalText_->SetColor({ 0.2f, 0.95f, 1.0f, 1.0f });

	// ホーミング弾タブ
	homingTabBgSprite_ = std::make_unique<Sprite>();
	homingTabBgSprite_->Initialize("white.png", { basePos_.x + 118.0f, basePos_.y }, { 0.0f, 0.0f });
	homingTabBgSprite_->SetSize({ 120.0f, 19.0f });
	homingTabBgSprite_->SetColor({ 0.02f, 0.05f, 0.08f, 0.50f });

	homingTabBorderSprite_ = std::make_unique<Sprite>();
	homingTabBorderSprite_->Initialize("white.png", { basePos_.x + 118.0f, basePos_.y }, { 0.0f, 0.0f });
	homingTabBorderSprite_->SetSize({ 120.0f, 1.0f });
	homingTabBorderSprite_->SetColor({ 0.3f, 0.4f, 0.5f, 0.30f });

	homingTabActiveLineSprite_ = std::make_unique<Sprite>();
	homingTabActiveLineSprite_->Initialize("white.png", { basePos_.x + 118.0f, basePos_.y + 17.0f }, { 0.0f, 0.0f });
	homingTabActiveLineSprite_->SetSize({ 120.0f, 2.0f });
	homingTabActiveLineSprite_->SetColor({ 1.0f, 0.80f, 0.20f, 0.0f }); // 初期非表示

	// ホーミング弾アイコン：照準ブラケット [ · ]
	homingTabIconBracketLSprite_ = std::make_unique<Sprite>();
	homingTabIconBracketLSprite_->Initialize("white.png", { basePos_.x + 125.0f, basePos_.y + 6.0f }, { 0.0f, 0.0f });
	homingTabIconBracketLSprite_->SetSize({ 2.0f, 8.0f });
	homingTabIconBracketLSprite_->SetColor({ 0.5f, 0.55f, 0.60f, 0.6f });

	homingTabIconDotSprite_ = std::make_unique<Sprite>();
	homingTabIconDotSprite_->Initialize("white.png", { basePos_.x + 130.0f, basePos_.y + 9.0f }, { 0.0f, 0.0f });
	homingTabIconDotSprite_->SetSize({ 2.0f, 2.0f });
	homingTabIconDotSprite_->SetColor({ 0.5f, 0.55f, 0.60f, 0.6f });

	homingTabIconBracketRSprite_ = std::make_unique<Sprite>();
	homingTabIconBracketRSprite_->Initialize("white.png", { basePos_.x + 135.0f, basePos_.y + 6.0f }, { 0.0f, 0.0f });
	homingTabIconBracketRSprite_->SetSize({ 2.0f, 8.0f });
	homingTabIconBracketRSprite_->SetColor({ 0.5f, 0.55f, 0.60f, 0.6f });

	homingText_ = std::make_unique<PixelText>();
	homingText_->Initialize("HOMING", { basePos_.x + 144.0f, basePos_.y + 4.0f }, 0.85f);
	homingText_->SetColor({ 0.5f, 0.55f, 0.60f, 0.6f });

	// 切替キー [E] バッジ
	keyBadgeBgSprite_ = std::make_unique<Sprite>();
	keyBadgeBgSprite_->Initialize("white.png", { basePos_.x + 242.0f, basePos_.y }, { 0.0f, 0.0f });
	keyBadgeBgSprite_->SetSize({ 48.0f, 19.0f });
	keyBadgeBgSprite_->SetColor({ 0.04f, 0.08f, 0.12f, 0.80f });

	keyBadgeBorderSprite_ = std::make_unique<Sprite>();
	keyBadgeBorderSprite_->Initialize("white.png", { basePos_.x + 242.0f, basePos_.y }, { 0.0f, 0.0f });
	keyBadgeBorderSprite_->SetSize({ 48.0f, 1.0f });
	keyBadgeBorderSprite_->SetColor({ 0.0f, 0.85f, 1.0f, 0.40f });

	keyText_ = std::make_unique<PixelText>();
	keyText_->Initialize("[E]", { basePos_.x + 248.0f, basePos_.y + 4.0f }, 0.85f);
	keyText_->SetColor({ 0.0f, 0.90f, 1.0f, 0.90f });

	// ------------------------------------------------------------
	// 2. 下段：ホーミング弾残弾数 ＆ リチャージカウントダウン
	// ------------------------------------------------------------
	const float lowerY = basePos_.y + 22.0f;

	bgSprite_ = std::make_unique<Sprite>();
	bgSprite_->Initialize("white.png", { basePos_.x, lowerY }, { 0.0f, 0.0f });
	bgSprite_->SetSize({ 290.0f, 44.0f });
	bgSprite_->SetColor({ 0.03f, 0.07f, 0.12f, 0.82f });

	leftAccentSprite_ = std::make_unique<Sprite>();
	leftAccentSprite_->Initialize("white.png", { basePos_.x, lowerY }, { 0.0f, 0.0f });
	leftAccentSprite_->SetSize({ 3.0f, 44.0f });
	leftAccentSprite_->SetColor({ 0.0f, 0.9f, 1.0f, 0.95f });

	topBorderSprite_ = std::make_unique<Sprite>();
	topBorderSprite_->Initialize("white.png", { basePos_.x, lowerY }, { 0.0f, 0.0f });
	topBorderSprite_->SetSize({ 290.0f, 1.0f });
	topBorderSprite_->SetColor({ 0.2f, 0.7f, 0.9f, 0.40f });

	bottomBorderSprite_ = std::make_unique<Sprite>();
	bottomBorderSprite_->Initialize("white.png", { basePos_.x, lowerY + 43.0f }, { 0.0f, 0.0f });
	bottomBorderSprite_->SetSize({ 290.0f, 1.0f });
	bottomBorderSprite_->SetColor({ 0.2f, 0.7f, 0.9f, 0.30f });

	divider1Sprite_ = std::make_unique<Sprite>();
	divider1Sprite_->Initialize("white.png", { basePos_.x + 48.0f, lowerY + 6.0f }, { 0.0f, 0.0f });
	divider1Sprite_->SetSize({ 1.0f, 32.0f });
	divider1Sprite_->SetColor({ 0.25f, 0.45f, 0.55f, 0.50f });

	divider2Sprite_ = std::make_unique<Sprite>();
	divider2Sprite_->Initialize("white.png", { basePos_.x + 168.0f, lowerY + 6.0f }, { 0.0f, 0.0f });
	divider2Sprite_->SetSize({ 1.0f, 32.0f });
	divider2Sprite_->SetColor({ 0.25f, 0.45f, 0.55f, 0.50f });

	// 残弾デジタル表示
	ammoLabelSprite_ = std::make_unique<Sprite>();
	ammoLabelSprite_->Initialize("white.png", { basePos_.x + 8.0f, lowerY + 6.0f }, { 0.0f, 0.0f });
	ammoLabelSprite_->SetSize({ 16.0f, 2.0f });
	ammoLabelSprite_->SetColor({ 0.0f, 0.85f, 1.0f, 0.8f });

	ammoDigitSprite_ = std::make_unique<Sprite>();
	ammoDigitSprite_->Initialize("numbers/5.png", { basePos_.x + 8.0f, lowerY + 10.0f }, { 0.0f, 0.0f });
	ammoDigitSprite_->SetSize({ 16.0f, 28.0f });
	ammoDigitSprite_->SetColor({ 0.2f, 0.95f, 1.0f, 1.0f });

	slashSprite_ = std::make_unique<Sprite>();
	slashSprite_->Initialize("white.png", { basePos_.x + 27.0f, lowerY + 14.0f }, { 0.0f, 0.0f });
	slashSprite_->SetSize({ 2.0f, 20.0f });
	slashSprite_->SetColor({ 0.35f, 0.55f, 0.65f, 0.5f });

	maxAmmoDigitSprite_ = std::make_unique<Sprite>();
	maxAmmoDigitSprite_->Initialize("numbers/5.png", { basePos_.x + 32.0f, lowerY + 20.0f }, { 0.0f, 0.0f });
	maxAmmoDigitSprite_->SetSize({ 10.0f, 18.0f });
	maxAmmoDigitSprite_->SetColor({ 0.45f, 0.65f, 0.75f, 0.7f });

	// 5連カートリッジスロット
	slotBgSprites_.resize(kMaxSlots);
	slotFillSprites_.resize(kMaxSlots);
	const float slotStartX = basePos_.x + 54.0f;
	const float slotY = lowerY + 11.0f;
	for (int i = 0; i < kMaxSlots; ++i) {
		const float x = slotStartX + static_cast<float>(i) * 22.0f;
		slotBgSprites_[i] = std::make_unique<Sprite>();
		slotBgSprites_[i]->Initialize("white.png", { x, slotY }, { 0.0f, 0.0f });
		slotBgSprites_[i]->SetSize({ 18.0f, 22.0f });
		slotBgSprites_[i]->SetColor({ 0.08f, 0.16f, 0.22f, 0.75f });

		slotFillSprites_[i] = std::make_unique<Sprite>();
		slotFillSprites_[i]->Initialize("white.png", { x + 2.0f, slotY + 2.0f }, { 0.0f, 0.0f });
		slotFillSprites_[i]->SetSize({ 14.0f, 18.0f });
		slotFillSprites_[i]->SetColor({ 0.1f, 0.95f, 1.0f, 0.95f });
	}

	// タイマー表示
	timerSecDigitSprite_ = std::make_unique<Sprite>();
	timerSecDigitSprite_->Initialize("numbers/0.png", { basePos_.x + 182.0f, lowerY + 7.0f }, { 0.0f, 0.0f });
	timerSecDigitSprite_->SetSize({ 12.0f, 21.0f });
	timerSecDigitSprite_->SetColor({ 1.0f, 0.85f, 0.25f, 1.0f });

	timerDotSprite_ = std::make_unique<Sprite>();
	timerDotSprite_->Initialize("white.png", { basePos_.x + 196.0f, lowerY + 24.0f }, { 0.0f, 0.0f });
	timerDotSprite_->SetSize({ 3.0f, 4.0f });
	timerDotSprite_->SetColor({ 1.0f, 0.85f, 0.25f, 1.0f });

	timerTenthDigitSprite_ = std::make_unique<Sprite>();
	timerTenthDigitSprite_->Initialize("numbers/0.png", { basePos_.x + 201.0f, lowerY + 7.0f }, { 0.0f, 0.0f });
	timerTenthDigitSprite_->SetSize({ 12.0f, 21.0f });
	timerTenthDigitSprite_->SetColor({ 1.0f, 0.85f, 0.25f, 1.0f });

	timerUnitSprite_ = std::make_unique<Sprite>();
	timerUnitSprite_->Initialize("white.png", { basePos_.x + 216.0f, lowerY + 14.0f }, { 0.0f, 0.0f });
	timerUnitSprite_->SetSize({ 4.0f, 12.0f });
	timerUnitSprite_->SetColor({ 0.7f, 0.6f, 0.2f, 0.7f });

	// 満タン表示用バッジ
	readyBadgeSprite_ = std::make_unique<Sprite>();
	readyBadgeSprite_->Initialize("white.png", { basePos_.x + 178.0f, lowerY + 10.0f }, { 0.0f, 0.0f });
	readyBadgeSprite_->SetSize({ 102.0f, 16.0f });
	readyBadgeSprite_->SetColor({ 0.1f, 0.85f, 0.35f, 0.85f });

	readyGlowSprite_ = std::make_unique<Sprite>();
	readyGlowSprite_->Initialize("white.png", { basePos_.x + 178.0f, lowerY + 8.0f }, { 0.0f, 0.0f });
	readyGlowSprite_->SetSize({ 102.0f, 2.0f });
	readyGlowSprite_->SetColor({ 0.4f, 1.0f, 0.6f, 0.95f });

	// 下部リチャージプログレスバー
	rechargeBarBgSprite_ = std::make_unique<Sprite>();
	rechargeBarBgSprite_->Initialize("white.png", { basePos_.x + 176.0f, lowerY + 32.0f }, { 0.0f, 0.0f });
	rechargeBarBgSprite_->SetSize({ 104.0f, 4.0f });
	rechargeBarBgSprite_->SetColor({ 0.08f, 0.16f, 0.24f, 0.7f });

	rechargeBarFillSprite_ = std::make_unique<Sprite>();
	rechargeBarFillSprite_->Initialize("white.png", { basePos_.x + 176.0f, lowerY + 32.0f }, { 0.0f, 0.0f });
	rechargeBarFillSprite_->SetSize({ 0.0f, 4.0f });
	rechargeBarFillSprite_->SetColor({ 1.0f, 0.75f, 0.15f, 0.95f });

	// ------------------------------------------------------------
	// 3. 画面中央：モード切り替え通知バナー
	// ------------------------------------------------------------
	const float bannerX = 460.0f;
	const float bannerY = 140.0f;
	const float bannerW = 360.0f;
	const float bannerH = 32.0f;

	switchBannerBgSprite_ = std::make_unique<Sprite>();
	switchBannerBgSprite_->Initialize("white.png", { bannerX, bannerY }, { 0.0f, 0.0f });
	switchBannerBgSprite_->SetSize({ bannerW, bannerH });
	switchBannerBgSprite_->SetColor({ 0.02f, 0.06f, 0.10f, 0.0f });

	switchBannerTopBorderSprite_ = std::make_unique<Sprite>();
	switchBannerTopBorderSprite_->Initialize("white.png", { bannerX, bannerY }, { 0.0f, 0.0f });
	switchBannerTopBorderSprite_->SetSize({ bannerW, 2.0f });
	switchBannerTopBorderSprite_->SetColor({ 0.0f, 0.9f, 1.0f, 0.0f });

	switchBannerBottomBorderSprite_ = std::make_unique<Sprite>();
	switchBannerBottomBorderSprite_->Initialize("white.png", { bannerX, bannerY + bannerH - 2.0f }, { 0.0f, 0.0f });
	switchBannerBottomBorderSprite_->SetSize({ bannerW, 2.0f });
	switchBannerBottomBorderSprite_->SetColor({ 0.0f, 0.9f, 1.0f, 0.0f });

	switchBannerNormalText_ = std::make_unique<PixelText>();
	switchBannerNormalText_->Initialize("<< NORMAL WEAPON >>", { bannerX + 44.0f, bannerY + 8.0f }, 1.25f);
	switchBannerNormalText_->SetColor({ 0.2f, 1.0f, 1.0f, 0.0f });

	switchBannerHomingText_ = std::make_unique<PixelText>();
	switchBannerHomingText_->Initialize("<< HOMING MISSILE >>", { bannerX + 38.0f, bannerY + 8.0f }, 1.25f);
	switchBannerHomingText_->SetColor({ 1.0f, 0.85f, 0.2f, 0.0f });

	ApplyPositions();
}

void HomingAmmoUI::SetPosition(const Vector2& pos) {
	basePos_ = pos;
	ApplyPositions();
}

void HomingAmmoUI::ApplyPositions() {
	if (!bgSprite_) return;

	// 1. 上段セレクター
	normalTabBgSprite_->SetPosition(basePos_);
	normalTabBorderSprite_->SetPosition(basePos_);
	normalTabActiveLineSprite_->SetPosition({ basePos_.x, basePos_.y + 17.0f });
	normalTabIconLineSprite_->SetPosition({ basePos_.x + 8.0f, basePos_.y + 9.0f });
	normalTabIconHeadSprite_->SetPosition({ basePos_.x + 18.0f, basePos_.y + 7.0f });
	normalText_->SetPosition({ basePos_.x + 28.0f, basePos_.y + 4.0f });

	homingTabBgSprite_->SetPosition({ basePos_.x + 118.0f, basePos_.y });
	homingTabBorderSprite_->SetPosition({ basePos_.x + 118.0f, basePos_.y });
	homingTabActiveLineSprite_->SetPosition({ basePos_.x + 118.0f, basePos_.y + 17.0f });
	homingTabIconBracketLSprite_->SetPosition({ basePos_.x + 125.0f, basePos_.y + 6.0f });
	homingTabIconDotSprite_->SetPosition({ basePos_.x + 130.0f, basePos_.y + 9.0f });
	homingTabIconBracketRSprite_->SetPosition({ basePos_.x + 135.0f, basePos_.y + 6.0f });
	homingText_->SetPosition({ basePos_.x + 144.0f, basePos_.y + 4.0f });

	keyBadgeBgSprite_->SetPosition({ basePos_.x + 242.0f, basePos_.y });
	keyBadgeBorderSprite_->SetPosition({ basePos_.x + 242.0f, basePos_.y });
	keyText_->SetPosition({ basePos_.x + 248.0f, basePos_.y + 4.0f });

	// 2. 下段残弾パネル
	const float lowerY = basePos_.y + 22.0f;
	bgSprite_->SetPosition({ basePos_.x, lowerY });
	leftAccentSprite_->SetPosition({ basePos_.x, lowerY });
	topBorderSprite_->SetPosition({ basePos_.x, lowerY });
	bottomBorderSprite_->SetPosition({ basePos_.x, lowerY + 43.0f });
	divider1Sprite_->SetPosition({ basePos_.x + 48.0f, lowerY + 6.0f });
	divider2Sprite_->SetPosition({ basePos_.x + 168.0f, lowerY + 6.0f });

	ammoLabelSprite_->SetPosition({ basePos_.x + 8.0f, lowerY + 6.0f });
	ammoDigitSprite_->SetPosition({ basePos_.x + 8.0f, lowerY + 10.0f });
	slashSprite_->SetPosition({ basePos_.x + 27.0f, lowerY + 14.0f });
	maxAmmoDigitSprite_->SetPosition({ basePos_.x + 32.0f, lowerY + 20.0f });

	const float slotStartX = basePos_.x + 54.0f;
	const float slotY = lowerY + 11.0f;
	for (int i = 0; i < kMaxSlots; ++i) {
		const float x = slotStartX + static_cast<float>(i) * 22.0f;
		if (slotBgSprites_[i]) {
			slotBgSprites_[i]->SetPosition({ x, slotY });
		}
		if (slotFillSprites_[i]) {
			slotFillSprites_[i]->SetPosition({ x + 2.0f, slotY + 2.0f });
		}
	}

	timerSecDigitSprite_->SetPosition({ basePos_.x + 182.0f, lowerY + 7.0f });
	timerDotSprite_->SetPosition({ basePos_.x + 196.0f, lowerY + 24.0f });
	timerTenthDigitSprite_->SetPosition({ basePos_.x + 201.0f, lowerY + 7.0f });
	timerUnitSprite_->SetPosition({ basePos_.x + 216.0f, lowerY + 14.0f });

	readyBadgeSprite_->SetPosition({ basePos_.x + 178.0f, lowerY + 10.0f });
	readyGlowSprite_->SetPosition({ basePos_.x + 178.0f, lowerY + 8.0f });

	rechargeBarBgSprite_->SetPosition({ basePos_.x + 176.0f, lowerY + 32.0f });
	rechargeBarFillSprite_->SetPosition({ basePos_.x + 176.0f, lowerY + 32.0f });
}

void HomingAmmoUI::Update(const Player* player) {
	if (!player) return;

	pulseTimer_ += 1.0f / 60.0f;
	const float pulse = 0.70f + 0.30f * std::sin(pulseTimer_ * 10.0f);

	// モード切り替え検知
	const bool newLockOnMode = player->GetIsLockOnMode();
	if (newLockOnMode != isLockOnMode_ || player->WasWeaponModeChanged()) {
		isLockOnMode_ = newLockOnMode;
		keyFlashTimer_ = 0.25f;
		switchBannerTimer_ = kSwitchBannerDuration;
	}

	// ------------------------------------------------------------
	// 1. 上段モードセレクタータブのスタイル更新
	// ------------------------------------------------------------
	if (!isLockOnMode_) {
		// 通常弾モード（アクティブ）
		normalTabBgSprite_->SetColor({ 0.04f, 0.24f, 0.32f, 0.88f });
		normalTabBorderSprite_->SetColor({ 0.0f, 0.95f, 1.0f, 0.85f });
		normalTabActiveLineSprite_->SetColor({ 0.0f, 1.0f, 1.0f, 1.0f });
		normalTabIconLineSprite_->SetColor({ 0.2f, 1.0f, 1.0f, 1.0f });
		normalTabIconHeadSprite_->SetColor({ 0.2f, 1.0f, 1.0f, 1.0f });
		normalText_->SetColor({ 0.3f, 1.0f, 1.0f, 1.0f });

		// ホーミング弾モード（スタンバイ）
		homingTabBgSprite_->SetColor({ 0.02f, 0.05f, 0.08f, 0.45f });
		homingTabBorderSprite_->SetColor({ 0.25f, 0.35f, 0.45f, 0.25f });
		homingTabActiveLineSprite_->SetColor({ 1.0f, 0.80f, 0.20f, 0.0f });
		homingTabIconBracketLSprite_->SetColor({ 0.40f, 0.48f, 0.55f, 0.45f });
		homingTabIconDotSprite_->SetColor({ 0.40f, 0.48f, 0.55f, 0.45f });
		homingTabIconBracketRSprite_->SetColor({ 0.40f, 0.48f, 0.55f, 0.45f });
		homingText_->SetColor({ 0.40f, 0.48f, 0.55f, 0.50f });

		// 下段ホーミングパネルの装飾（スタンバイ表示）
		bgSprite_->SetColor({ 0.02f, 0.05f, 0.08f, 0.70f });
		leftAccentSprite_->SetColor({ 0.2f, 0.50f, 0.65f, 0.70f });
		topBorderSprite_->SetColor({ 0.15f, 0.4f, 0.5f, 0.25f });
	} else {
		// 通常弾モード（スタンバイ）
		normalTabBgSprite_->SetColor({ 0.02f, 0.05f, 0.08f, 0.45f });
		normalTabBorderSprite_->SetColor({ 0.25f, 0.35f, 0.45f, 0.25f });
		normalTabActiveLineSprite_->SetColor({ 0.0f, 1.0f, 1.0f, 0.0f });
		normalTabIconLineSprite_->SetColor({ 0.40f, 0.48f, 0.55f, 0.45f });
		normalTabIconHeadSprite_->SetColor({ 0.40f, 0.48f, 0.55f, 0.45f });
		normalText_->SetColor({ 0.40f, 0.48f, 0.55f, 0.50f });

		// ホーミング弾モード（アクティブ）
		homingTabBgSprite_->SetColor({ 0.30f, 0.18f, 0.04f, 0.88f });
		homingTabBorderSprite_->SetColor({ 1.0f, 0.80f, 0.15f, 0.85f });
		homingTabActiveLineSprite_->SetColor({ 1.0f, 0.85f, 0.20f, 1.0f });
		homingTabIconBracketLSprite_->SetColor({ 1.0f, 0.85f, 0.25f, pulse });
		homingTabIconDotSprite_->SetColor({ 1.0f, 0.85f, 0.25f, pulse });
		homingTabIconBracketRSprite_->SetColor({ 1.0f, 0.85f, 0.25f, pulse });
		homingText_->SetColor({ 1.0f, 0.85f, 0.25f, 1.0f });

		// 下段ホーミングパネルの装飾（アクティブ全開発光）
		bgSprite_->SetColor({ 0.04f, 0.08f, 0.14f, 0.88f });
		leftAccentSprite_->SetColor({ 1.0f, 0.80f, 0.15f, 1.0f });
		topBorderSprite_->SetColor({ 1.0f, 0.75f, 0.20f, 0.50f });
	}

	// [E] キーバッジのフラッシュ演出
	if (keyFlashTimer_ > 0.0f) {
		keyFlashTimer_ -= 1.0f / 60.0f;
		const float t = (std::max)(0.0f, keyFlashTimer_ / 0.25f);
		keyBadgeBgSprite_->SetColor({ 0.8f * t + 0.04f, 0.8f * t + 0.08f, 0.9f * t + 0.12f, 0.95f });
		keyText_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	} else {
		keyBadgeBgSprite_->SetColor({ 0.04f, 0.08f, 0.12f, 0.80f });
		keyText_->SetColor({ 0.0f, 0.90f, 1.0f, 0.90f });
	}

	// ------------------------------------------------------------
	// 2. 下段ホーミング弾残弾数 ＆ リチャージ更新
	// ------------------------------------------------------------
	const int ammo = std::clamp(player->GetHomingAmmo(), 0, kMaxSlots);
	const int maxAmmo = player->GetMaxHomingAmmo();
	const float progress = std::clamp(player->GetHomingRechargeProgress(), 0.0f, 1.0f);
	const float remainingTime = player->GetHomingRemainingTime();

	isFull_ = (ammo >= maxAmmo);

	if (ammo != prevAmmo_) {
		ammoDigitSprite_->SetTexture("numbers/" + std::to_string(ammo) + ".png");
		if (ammo == 0) {
			ammoDigitSprite_->SetColor({ 1.0f, 0.25f, 0.25f, 1.0f });
		} else if (ammo <= 2) {
			ammoDigitSprite_->SetColor({ 1.0f, 0.80f, 0.20f, 1.0f });
		} else {
			ammoDigitSprite_->SetColor({ 0.2f, 0.95f, 1.0f, 1.0f });
		}
		prevAmmo_ = ammo;
	}

	for (int i = 0; i < kMaxSlots; ++i) {
		if (i < ammo) {
			slotFillSprites_[i]->SetSize({ 14.0f, 18.0f });
			slotFillSprites_[i]->SetColor(isLockOnMode_ ? Vector4{ 1.0f, 0.85f, 0.25f, 0.95f } : Vector4{ 0.1f, 0.95f, 1.0f, 0.85f });
		} else if (i == ammo && !isFull_) {
			slotFillSprites_[i]->SetSize({ 14.0f, 18.0f * progress });
			slotFillSprites_[i]->SetColor({ 1.0f, 0.75f, 0.15f, pulse });
		} else {
			slotFillSprites_[i]->SetSize({ 14.0f, 0.0f });
		}
		slotBgSprites_[i]->Update();
		slotFillSprites_[i]->Update();
	}

	const float kBarMaxWidth = 104.0f;
	if (isFull_) {
		rechargeBarFillSprite_->SetSize({ kBarMaxWidth, 4.0f });
		rechargeBarFillSprite_->SetColor({ 0.15f, 0.95f, 0.40f, 0.95f });

		const float readyAlpha = 0.75f + 0.25f * std::sin(pulseTimer_ * 6.0f);
		readyBadgeSprite_->SetColor({ 0.1f, 0.85f, 0.35f, readyAlpha });
		readyBadgeSprite_->Update();
		readyGlowSprite_->Update();
	} else {
		rechargeBarFillSprite_->SetSize({ kBarMaxWidth * progress, 4.0f });
		rechargeBarFillSprite_->SetColor({ 1.0f, 0.75f, 0.15f, 0.95f });

		const int sec = static_cast<int>(remainingTime);
		const int tenth = static_cast<int>((remainingTime - static_cast<float>(sec)) * 10.0f) % 10;

		if (sec != prevSec_) {
			timerSecDigitSprite_->SetTexture("numbers/" + std::to_string(std::clamp(sec, 0, 9)) + ".png");
			prevSec_ = sec;
		}
		if (tenth != prevTenth_) {
			timerTenthDigitSprite_->SetTexture("numbers/" + std::to_string(std::clamp(tenth, 0, 9)) + ".png");
			prevTenth_ = tenth;
		}

		timerSecDigitSprite_->Update();
		timerDotSprite_->Update();
		timerTenthDigitSprite_->Update();
		timerUnitSprite_->Update();
	}

	// ------------------------------------------------------------
	// 3. 画面中央：モード切り替え通知バナー更新
	// ------------------------------------------------------------
	if (switchBannerTimer_ > 0.0f) {
		switchBannerTimer_ -= 1.0f / 60.0f;
		float alpha = 1.0f;
		if (switchBannerTimer_ > kSwitchBannerDuration - 0.15f) {
			// フェードイン (0.15秒)
			alpha = (kSwitchBannerDuration - switchBannerTimer_) / 0.15f;
		} else if (switchBannerTimer_ < 0.25f) {
			// フェードアウト (0.25秒)
			alpha = switchBannerTimer_ / 0.25f;
		}
		alpha = std::clamp(alpha, 0.0f, 1.0f);

		if (!isLockOnMode_) {
			// 通常弾切り替えバナー
			switchBannerBgSprite_->SetColor({ 0.02f, 0.08f, 0.14f, 0.85f * alpha });
			switchBannerTopBorderSprite_->SetColor({ 0.0f, 0.95f, 1.0f, 0.95f * alpha });
			switchBannerBottomBorderSprite_->SetColor({ 0.0f, 0.95f, 1.0f, 0.95f * alpha });
			switchBannerNormalText_->SetColor({ 0.2f, 1.0f, 1.0f, alpha });
			switchBannerNormalText_->SetVisible(true);
			switchBannerHomingText_->SetVisible(false);
		} else {
			// ホーミング弾切り替えバナー
			switchBannerBgSprite_->SetColor({ 0.18f, 0.08f, 0.02f, 0.85f * alpha });
			switchBannerTopBorderSprite_->SetColor({ 1.0f, 0.80f, 0.15f, 0.95f * alpha });
			switchBannerBottomBorderSprite_->SetColor({ 1.0f, 0.80f, 0.15f, 0.95f * alpha });
			switchBannerHomingText_->SetColor({ 1.0f, 0.85f, 0.2f, alpha });
			switchBannerHomingText_->SetVisible(true);
			switchBannerNormalText_->SetVisible(false);
		}

		switchBannerBgSprite_->Update();
		switchBannerTopBorderSprite_->Update();
		switchBannerBottomBorderSprite_->Update();
		switchBannerNormalText_->Update();
		switchBannerHomingText_->Update();
	}

	// 各種スプライト更新
	normalTabBgSprite_->Update();
	normalTabBorderSprite_->Update();
	normalTabActiveLineSprite_->Update();
	normalTabIconLineSprite_->Update();
	normalTabIconHeadSprite_->Update();
	normalText_->Update();

	homingTabBgSprite_->Update();
	homingTabBorderSprite_->Update();
	homingTabActiveLineSprite_->Update();
	homingTabIconBracketLSprite_->Update();
	homingTabIconDotSprite_->Update();
	homingTabIconBracketRSprite_->Update();
	homingText_->Update();

	keyBadgeBgSprite_->Update();
	keyBadgeBorderSprite_->Update();
	keyText_->Update();

	bgSprite_->Update();
	leftAccentSprite_->Update();
	topBorderSprite_->Update();
	bottomBorderSprite_->Update();
	divider1Sprite_->Update();
	divider2Sprite_->Update();
	ammoLabelSprite_->Update();
	ammoDigitSprite_->Update();
	slashSprite_->Update();
	maxAmmoDigitSprite_->Update();
	rechargeBarBgSprite_->Update();
	rechargeBarFillSprite_->Update();
}

void HomingAmmoUI::Draw() {
	if (!isVisible_) return;

	// 1. 上段モードセレクタータブ
	normalTabBgSprite_->Draw();
	normalTabBorderSprite_->Draw();
	normalTabActiveLineSprite_->Draw();
	normalTabIconLineSprite_->Draw();
	normalTabIconHeadSprite_->Draw();
	normalText_->Draw();

	homingTabBgSprite_->Draw();
	homingTabBorderSprite_->Draw();
	homingTabActiveLineSprite_->Draw();
	homingTabIconBracketLSprite_->Draw();
	homingTabIconDotSprite_->Draw();
	homingTabIconBracketRSprite_->Draw();
	homingText_->Draw();

	keyBadgeBgSprite_->Draw();
	keyBadgeBorderSprite_->Draw();
	keyText_->Draw();

	// 2. 下段ホーミング弾残弾パネル
	bgSprite_->Draw();
	leftAccentSprite_->Draw();
	topBorderSprite_->Draw();
	bottomBorderSprite_->Draw();
	divider1Sprite_->Draw();
	divider2Sprite_->Draw();

	ammoLabelSprite_->Draw();
	ammoDigitSprite_->Draw();
	slashSprite_->Draw();
	maxAmmoDigitSprite_->Draw();

	for (int i = 0; i < kMaxSlots; ++i) {
		slotBgSprites_[i]->Draw();
		slotFillSprites_[i]->Draw();
	}

	rechargeBarBgSprite_->Draw();
	rechargeBarFillSprite_->Draw();

	if (isFull_) {
		readyBadgeSprite_->Draw();
		readyGlowSprite_->Draw();
	} else {
		timerSecDigitSprite_->Draw();
		timerDotSprite_->Draw();
		timerTenthDigitSprite_->Draw();
		timerUnitSprite_->Draw();
	}

	// 3. 中央モード切り替えバナー演出
	if (switchBannerTimer_ > 0.0f) {
		switchBannerBgSprite_->Draw();
		switchBannerTopBorderSprite_->Draw();
		switchBannerBottomBorderSprite_->Draw();
		if (!isLockOnMode_) {
			switchBannerNormalText_->Draw();
		} else {
			switchBannerHomingText_->Draw();
		}
	}
}
