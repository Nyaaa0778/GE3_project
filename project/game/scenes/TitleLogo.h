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
/// タイトルシーン用 サイバーパンク・ベクターテキストロゴ「CYBERAIL」
/// 都市と同じワイヤーフレーム枝分かれ形成演出 ＆ 視認性確保バックプレート
/// </summary>
class TitleLogo {
public:
	TitleLogo();
	~TitleLogo();

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="customCamera">指定があればそのカメラを使用。nullptrなら専用HUD正面カメラを自動作成</param>
	void Initialize(Camera* customCamera = nullptr);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画（遮蔽プレート ➔ ワイヤーフレーム文字 ➔ 装飾フレーム）
	/// </summary>
	void Draw();

	/// <summary>
	/// 形成アニメーションのリセット
	/// </summary>
	void ResetAnimation();

	/// <summary>
	/// 表示位置（3D空間座標）の設定 / 取得
	/// </summary>
	void SetPosition(const Vector3& position);
	const Vector3& GetPosition() const { return rootPosition_; }

	/// <summary>
	/// スケールの設定 / 取得
	/// </summary>
	void SetScale(const Vector3& scale);
	const Vector3& GetScale() const { return rootScale_; }

	/// <summary>
	/// 回転角度の設定 / 取得（立体パース・見下ろし角）
	/// </summary>
	void SetRotation(const Vector3& rotation);
	const Vector3& GetRotation() const { return rootRotation_; }

	/// <summary>
	/// 浮遊（ホバー・レビテーション）の有効/無効
	/// </summary>
	void SetHoverEnabled(bool enabled) { enableHover_ = enabled; }
	bool IsHoverEnabled() const { return enableHover_; }

	/// <summary>
	/// バックプレート（暗幕）の表示/非表示
	/// </summary>
	void SetBackdropEnabled(bool enabled) { enableBackdrop_ = enabled; }
	bool IsBackdropEnabled() const { return enableBackdrop_; }

	/// <summary>
	/// バックプレートの透明度 (0.0: 透明 〜 1.0: 完全な黒)
	/// </summary>
	void SetBackdropAlpha(float alpha);
	float GetBackdropAlpha() const { return backdropAlpha_; }

	// サブテキスト位置設定 / 取得
	void SetPromptPosition(const Vector3& position);
	const Vector3& GetPromptPosition() const { return promptPosition_; }

	// 全文字（メイン＋サブプロンプト）の形成完了フラグ
	bool IsAllBuilt() const { return isAllBuilt_; }
	bool IsPromptBuilt() const { return isPromptBuilt_; }

private:
	// 辺の伸長アニメーションパラメータ
	struct EdgeGrowth {
		int fromVertex = 0;
		int toVertex = 0;
		float startTime = 0.0f;
		float duration = 0.20f;
	};

	// 1文字分のデータ
	struct LetterData {
		char character = ' ';
		float width = 1.0f;
		float xOffset = 0.0f;
		std::vector<Vector3> localVertices;
		std::vector<std::pair<int, int>> baseEdges;
		std::vector<EdgeGrowth> edgeGrowths;
		std::unique_ptr<WireframeObject> wireObject;

		float startDelay = 0.0f;
		bool isComplete = false;
		float flashTimer = 0.0f;
		Vector4 baseColor = {0.0f, 0.90f, 1.0f, 1.0f}; // ネオンシアン
	};

	void BuildLetterMeshes();
	void BuildPromptMeshes();
	void SetupGrowthAnimation();
	void UpdateGrowthAnimation();
	void UpdatePromptAnimation();

	// 文字ジオメトリ生成ヘルパー（任意のサイズに対応）
	void GenerateCharMesh(char c, float w, float h, float t, LetterData& l);

private:
	// 専用HUD正面カメラ（外部カメラが指定されない場合に使用）
	std::unique_ptr<Camera> hudCamera_;
	Camera* activeCamera_ = nullptr;

	// 背面遮蔽プレート（背景都市の線を減衰させて文字を浮き立たせる）
	std::unique_ptr<Sprite> backdropSprite_;
	bool enableBackdrop_ = true;
	float backdropAlpha_ = 0.88f;

	// 外枠のサイバー装飾フレーム用ワイヤーオブジェクト（ブラケット [ ] や上下ライン）
	std::unique_ptr<WireframeObject> frameObject_;

	// メインロゴ「CYBERAIL」の文字データ
	std::vector<LetterData> letters_;

	// サブプロンプト「PRESS SPACE TO START」の文字データと装飾
	std::vector<LetterData> promptLetters_;
	std::unique_ptr<Sprite> promptBackdrop_;
	std::unique_ptr<WireframeObject> promptFrameObject_;
	Vector3 promptPosition_ = {0.0f, -0.55f, 0.0f};
	float promptAnimTime_ = 0.0f;
	bool isPromptBuilt_ = false;
	float promptBlinkTimer_ = 0.0f;

	// アニメーション制御
	float animTime_ = 0.0f;
	bool isAllBuilt_ = false;
	float pulseTimer_ = 0.0f;

	// 全体の配置（画面上寄りに上品なサイズで配置）
	Vector3 rootPosition_ = {0.0f, 1.25f, 0.0f};
	Vector3 rootScale_ = {0.68f, 0.68f, 0.68f};
	Vector3 rootRotation_ = {0.16f, -0.06f, 0.0f}; // 奥行きを感じさせる微小チルト角 (見下ろし＋斜め)
	bool enableHover_ = true;                       // 浮遊アニメーション
	float extrusionDepth_ = 0.28f;                  // 立体押し出しの奥行き厚み
};
