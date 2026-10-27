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
/// ゲームオーバー演出用 サイバーUI
/// 画面の暗転、GAMEOVERのベクター文字描画、サイバーグリッチ、操作プロンプトを管理
/// </summary>
class GameOverUI {
public:
	GameOverUI();
	~GameOverUI();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// 演出開始（撃破時に呼び出し）
	/// </summary>
	void Start();

	/// <summary>
	/// 演出リセット（通常状態への復帰）
	/// </summary>
	void Reset();

	/// <summary>
	/// アクティブ状態か
	/// </summary>
	bool IsActive() const { return isActive_; }

	/// <summary>
	/// リトライ（ゲームプレイ再開）が要求されたか
	/// </summary>
	bool IsRetryRequested() const { return isRetryRequested_; }

	/// <summary>
	/// タイトルへ戻ることが要求されたか
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
		std::unique_ptr<WireframeObject> ghostWireObject; // グリッチ色収差用（シアンゴースト）
	};

	void BuildMainLetters();
	void BuildSubLetters();
	void BuildFrame();
	void GenerateCharMesh(char c, float w, float h, float t, LetterData& l);

private:
	bool isActive_ = false;
	float timer_ = 0.0f;

	// 背景暗幕スプライト（背景を薄暗くする）
	std::unique_ptr<Sprite> backdropSprite_;
	float backdropAlpha_ = 0.0f;
	static constexpr float kTargetBackdropAlpha = 0.68f;
	static constexpr float kBackdropFadeDuration = 0.90f;

	// 専用HUD正面カメラ
	std::unique_ptr<Camera> hudCamera_;

	// メインロゴ "GAME OVER"
	std::vector<LetterData> mainLetters_;
	Vector3 mainPosition_ = { 0.0f, 0.35f, 0.0f };
	Vector3 mainScale_ = { 0.70f, 0.70f, 0.70f };
	float mainExtrusionDepth_ = 0.22f;

	// メイン外枠サイバーフレーム [  ]
	std::unique_ptr<WireframeObject> frameObject_;
	std::unique_ptr<WireframeObject> frameGhostObject_;

	// 上部サブタイトル "SYSTEM FAILURE // CRITICAL DAMAGE"
	std::vector<LetterData> headerLetters_;
	Vector3 headerPosition_ = { 0.0f, 1.25f, 0.0f };
	Vector3 headerScale_ = { 0.28f, 0.28f, 0.28f };

	// 下部プロンプト "ENTER : RETRY    SPACE : TITLE"
	std::vector<LetterData> promptLetters_;
	Vector3 promptPosition_ = { 0.0f, -0.90f, 0.0f };
	Vector3 promptScale_ = { 0.30f, 0.30f, 0.30f };

	// グリッチ演出制御パラメータ
	bool isGlitching_ = false;
	float glitchBurstTimer_ = 0.0f;
	float glitchBurstDuration_ = 0.08f;
	float glitchIntervalTimer_ = 0.0f;
	float glitchNextInterval_ = 0.5f;
	float currentGlitchIntensity_ = 0.0f;
	float currentChromaticAberration_ = 0.0f;
	Vector3 jitterOffset_ = { 0.0f, 0.0f, 0.0f };

	// 入力要求フラグ
	bool isRetryRequested_ = false;
	bool isTitleRequested_ = false;
};
