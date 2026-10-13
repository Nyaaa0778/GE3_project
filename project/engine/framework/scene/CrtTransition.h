#pragma once

#include "PostProcessRenderer.h"
#include <string>
#include <memory>

class CrtTransition {
public:
	enum class State {
		kIdle,          // 待機中（演出なし）
		kGlitchBurst,   // 激しいRGBグリッチ
		kShutdownY,     // 上下から中央へ急激に潰れ、水平白熱ラインへ
		kShutdownX,     // 水平ラインが左右から中央へ収縮してドットへ
		kBlackout,      // 中心の輝点がプツンと消えて暗転（この瞬間にシーン切替）
		kTurnOnDot,     // 新シーン: 中心の輝点がパッと点灯
		kTurnOnX,       // 新シーン: 水平に一気に展開
		kTurnOnY,       // 新シーン: 上下に一気に展開して全画面へ
	};

	static CrtTransition* GetInstance();
	static void Finalize();

	void Initialize();
	void Update();

	/// <summary>
	/// シーン遷移演出を開始（RGBグリッチ -> CRTシャットダウン -> シーン切替 -> CRT再点灯）
	/// </summary>
	/// <param name="nextScene">遷移先のシーン名（"GAMEPLAY", "TITLE" など）</param>
	/// <param name="postModeAfter">遷移完了後に復帰させるポストプロセスモード</param>
	void StartTransition(
		const std::string& nextScene,
		PostProcessRenderer::PostProcessMode postModeAfter = PostProcessRenderer::PostProcessMode::kNormal);

	/// <summary>
	/// 遷移中かどうか（入力無効化や多重起動防止用）
	/// </summary>
	bool IsTransitioning() const { return state_ != State::kIdle; }

	/// <summary>
	/// 現在のステート
	/// </summary>
	State GetState() const { return state_; }

	/// <summary>
	/// ImGuiデバッグ表示
	/// </summary>
	void UpdateImGui();

	// 演出パラメータ設定
	void SetGlitchDuration(float duration) { glitchDuration_ = duration; }
	float GetGlitchDuration() const { return glitchDuration_; }

	void SetBlackoutDuration(float duration) { blackoutDuration_ = duration; }
	float GetBlackoutDuration() const { return blackoutDuration_; }

	friend std::default_delete<CrtTransition>;

private:
	CrtTransition() = default;
	~CrtTransition() = default;
	CrtTransition(const CrtTransition&) = delete;
	CrtTransition& operator=(const CrtTransition&) = delete;

	static std::unique_ptr<CrtTransition> instance_;

	State state_ = State::kIdle;
	float timer_ = 0.0f;
	std::string nextSceneName_;

	// 演出パラメータ
	float glitchDuration_ = 0.70f;       // RGBグリッチ時間（秒、デフォルト0.70秒）
	float shutdownYDuration_ = 0.15f;    // 上下潰れ時間（秒）
	float shutdownXDuration_ = 0.12f;    // 左右収縮時間（秒）
	float blackoutDuration_ = 0.40f;     // 完全暗転待機時間（秒、デフォルト0.40秒）
	float turnOnDotDuration_ = 0.04f;    // 点灯時間（秒）
	float turnOnXDuration_ = 0.10f;      // 水平展開時間（秒）
	float turnOnYDuration_ = 0.12f;      // 垂直展開時間（秒）
	PostProcessRenderer::PostProcessMode postModeAfter_ = PostProcessRenderer::PostProcessMode::kNormal;
};
