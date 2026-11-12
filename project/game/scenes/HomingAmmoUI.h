#pragma once

#include <memory>
#include <vector>
#include <string>
#include "Vector2.h"
#include "Vector4.h"

class Sprite;
class Player;

/// <summary>
/// サイバーパンク風ピクセル文字ジェネレータ（Sprite矩形による高解像度幾何学フォント）
/// </summary>
class PixelText {
public:
	PixelText();
	~PixelText();

	void Initialize(const std::string& text, const Vector2& pos, float scale = 1.0f);
	void SetPosition(const Vector2& pos);
	void SetColor(const Vector4& color);
	void Update();
	void Draw();
	void SetVisible(bool visible) { isVisible_ = visible; }
	float GetTotalWidth() const { return totalWidth_; }

private:
	struct Segment {
		std::unique_ptr<Sprite> sprite;
		Vector2 offset;
		Vector2 size;
	};
	std::vector<Segment> segments_;
	Vector2 basePos_ = { 0.0f, 0.0f };
	float scale_ = 1.0f;
	float totalWidth_ = 0.0f;
	bool isVisible_ = true;
};

/// <summary>
/// 武器モード（通常弾／ホーミング弾）切り替えインジケーター & 弾薬リチャージHUD
/// </summary>
class HomingAmmoUI {
public:
	HomingAmmoUI();
	~HomingAmmoUI();

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="basePos">UI全体の左上座標</param>
	void Initialize(const Vector2& basePos = { 12.0f, 58.0f });

	/// <summary>
	/// 更新
	/// </summary>
	/// <param name="player">プレイヤー参照</param>
	void Update(const Player* player);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// 座標の更新
	/// </summary>
	void SetPosition(const Vector2& pos);
	const Vector2& GetPosition() const { return basePos_; }

	/// <summary>
	/// 表示・非表示
	/// </summary>
	void SetVisible(bool visible) { isVisible_ = visible; }
	bool IsVisible() const { return isVisible_; }

private:
	void ApplyPositions();

private:
	Vector2 basePos_ = { 12.0f, 58.0f };
	bool isVisible_ = true;

	// ============================================================
	// 1. 上段：武器モードセレクターバー (NORMAL / HOMING / [E])
	// ============================================================
	// 通常弾タブ
	std::unique_ptr<Sprite> normalTabBgSprite_;
	std::unique_ptr<Sprite> normalTabBorderSprite_;
	std::unique_ptr<Sprite> normalTabActiveLineSprite_;
	std::unique_ptr<Sprite> normalTabIconLineSprite_;
	std::unique_ptr<Sprite> normalTabIconHeadSprite_;
	std::unique_ptr<PixelText> normalText_;

	// ホーミング弾タブ
	std::unique_ptr<Sprite> homingTabBgSprite_;
	std::unique_ptr<Sprite> homingTabBorderSprite_;
	std::unique_ptr<Sprite> homingTabActiveLineSprite_;
	std::unique_ptr<Sprite> homingTabIconBracketLSprite_;
	std::unique_ptr<Sprite> homingTabIconDotSprite_;
	std::unique_ptr<Sprite> homingTabIconBracketRSprite_;
	std::unique_ptr<PixelText> homingText_;

	// 切替キー [E] バッジ
	std::unique_ptr<Sprite> keyBadgeBgSprite_;
	std::unique_ptr<Sprite> keyBadgeBorderSprite_;
	std::unique_ptr<PixelText> keyText_;
	float keyFlashTimer_ = 0.0f; // Eキー押下時の白フラッシュ

	// ============================================================
	// 2. 下段：ホーミング弾残弾数 ＆ リチャージカウントダウン
	// ============================================================
	std::unique_ptr<Sprite> bgSprite_;
	std::unique_ptr<Sprite> leftAccentSprite_;
	std::unique_ptr<Sprite> topBorderSprite_;
	std::unique_ptr<Sprite> bottomBorderSprite_;
	std::unique_ptr<Sprite> divider1Sprite_;
	std::unique_ptr<Sprite> divider2Sprite_;

	// 残弾デジタル表示
	std::unique_ptr<Sprite> ammoDigitSprite_;
	std::unique_ptr<Sprite> slashSprite_;
	std::unique_ptr<Sprite> maxAmmoDigitSprite_;
	std::unique_ptr<Sprite> ammoLabelSprite_;

	// 5連カートリッジスロット
	static constexpr int kMaxSlots = 5;
	std::vector<std::unique_ptr<Sprite>> slotBgSprites_;
	std::vector<std::unique_ptr<Sprite>> slotFillSprites_;

	// リチャージタイマー (例: 1 . 8 s)
	std::unique_ptr<Sprite> timerSecDigitSprite_;
	std::unique_ptr<Sprite> timerDotSprite_;
	std::unique_ptr<Sprite> timerTenthDigitSprite_;
	std::unique_ptr<Sprite> timerUnitSprite_;

	// 満タン表示用バッジ
	std::unique_ptr<Sprite> readyBadgeSprite_;
	std::unique_ptr<Sprite> readyGlowSprite_;

	// 下部リチャージプログレスバー
	std::unique_ptr<Sprite> rechargeBarBgSprite_;
	std::unique_ptr<Sprite> rechargeBarFillSprite_;

	// ============================================================
	// 3. 画面中央：モード切り替え通知バナー (MODE SWITCH BANNER)
	// ============================================================
	std::unique_ptr<Sprite> switchBannerBgSprite_;
	std::unique_ptr<Sprite> switchBannerTopBorderSprite_;
	std::unique_ptr<Sprite> switchBannerBottomBorderSprite_;
	std::unique_ptr<PixelText> switchBannerNormalText_;
	std::unique_ptr<PixelText> switchBannerHomingText_;
	float switchBannerTimer_ = 0.0f;
	static constexpr float kSwitchBannerDuration = 0.90f;

	// 内部状態・キャッシュ
	bool isLockOnMode_ = false; // 現在の選択モード
	int prevAmmo_ = -1;
	int prevSec_ = -1;
	int prevTenth_ = -1;
	float pulseTimer_ = 0.0f;
	bool isFull_ = true;
};
