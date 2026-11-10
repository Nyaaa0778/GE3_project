#pragma once

#include <vector>
#include <memory>
#include <string>
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

class Camera;
class WireframeObject;
class Sprite;

/// <summary>
/// ゲームクリア（リザルト）演出用 サイバーUI
/// TitleLogo同様の3D立体押し出しタイトル ＆ コンパクトで洗練されたネオン数字スプライト
/// </summary>
class GameClearUI {
public:
	GameClearUI();
	~GameClearUI();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 3D描画（HUDカメラによる立体タイトル文字・ラベル・外枠）
	/// </summary>
	void Draw3D();

	/// <summary>
	/// 2D描画（数字スプライト・背景遮蔽プレート）
	/// </summary>
	void Draw2D();

	/// <summary>
	/// 演出開始（クリア時に呼び出し）
	/// </summary>
	void Start(int finalScore, int killedCount);

	/// <summary>
	/// アクティブ状態か
	/// </summary>
	bool IsActive() const { return isActive_; }

	/// <summary>
	/// タイトルへ戻ることが要求されたか（ENTER/SPACE入力）
	/// </summary>
	bool IsTitleRequested() const { return isTitleRequested_; }

	/// <summary>
	/// 背景暗幕の現在のアルファ値
	/// </summary>
	float GetBackdropAlpha() const { return backdropAlpha_; }

private:
	struct LetterData {
		char character = ' ';
		float width = 1.0f;
		float xOffset = 0.0f;
		std::vector<Vector3> localVertices;
		std::vector<std::pair<int, int>> baseEdges;
		std::unique_ptr<WireframeObject> wireObject;
	};

	// 3D立体タイトル「STAGE CLEAR」の構築
	void Build3DTitleLetters();
	// 小さく端正なサブラベル構築
	void BuildSubTextLetters(const std::string& text, std::vector<LetterData>& letterList, float h, float t, float tracking, const Vector3& basePos);
	void BuildFrame();
	void GenerateCharMesh(char c, float w, float h, float t, LetterData& l);
	void UpdateDigitSprites(std::vector<std::unique_ptr<Sprite>>& sprites, int value, float alpha, const Vector4& baseColor);

private:
	bool isActive_ = false;
	float timer_ = 0.0f;

	// 目標値と現在のカウントアップ表示値
	int targetScore_ = 0;
	int displayScore_ = 0;
	int targetKills_ = 0;
	int displayKills_ = 0;
	bool isCountUpFinished_ = false;

	// 全画面背景暗幕スプライト
	std::unique_ptr<Sprite> backdropSprite_;
	float backdropAlpha_ = 0.0f;
	static constexpr float kTargetBackdropAlpha = 0.72f;

	// 専用HUD正面カメラ
	std::unique_ptr<Camera> hudCamera_;

	// メインタイトル "STAGE CLEAR"（3D立体押し出し）
	std::vector<LetterData> titleLetters_;
	Vector3 titlePosition_ = { 0.0f, 1.25f, 0.0f };
	Vector3 titleScale_ = { 0.48f, 0.48f, 0.48f };
	Vector3 titleRotation_ = { 0.16f, -0.05f, 0.0f }; // 立体見下ろし角
	float extrusionDepth_ = 0.28f;

	// サブラベル "SCORE"
	std::vector<LetterData> scoreLabelLetters_;
	Vector3 scoreLabelPosition_ = { -1.45f, 0.08f, 0.0f };

	// サブラベル "TARGETS"
	std::vector<LetterData> targetLabelLetters_;
	Vector3 targetLabelPosition_ = { -1.45f, -0.42f, 0.0f };

	// 操作プロンプト "SPACE : TITLE"
	std::vector<LetterData> promptLetters_;
	Vector3 promptPosition_ = { 0.0f, -1.35f, 0.0f };

	// サイバー外枠フレーム
	std::unique_ptr<WireframeObject> frameObject_;

	// 数字スプライト群（コンパクトサイズ）
	static constexpr int kScoreDigits = 6;
	static constexpr int kKillDigits = 3;
	std::vector<std::unique_ptr<Sprite>> scoreDigits_;
	std::vector<std::unique_ptr<Sprite>> killDigits_;

	// 数字スプライト用スリムプレート
	std::unique_ptr<Sprite> scorePlateSprite_;
	std::unique_ptr<Sprite> killPlateSprite_;

	// タイトルへ戻る入力要求
	bool isTitleRequested_ = false;
};
