#include "CrtTransition.h"

#include "SceneManager.h"
#include "PostProcessRenderer.h"
#include "PostProcessEffects.h"

#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

std::unique_ptr<CrtTransition> CrtTransition::instance_ = nullptr;

CrtTransition* CrtTransition::GetInstance() {
	if (!instance_) {
		instance_ = std::unique_ptr<CrtTransition>(new CrtTransition());
	}
	return instance_.get();
}

void CrtTransition::Finalize() {
	instance_.reset();
}

void CrtTransition::Initialize() {
	state_ = State::kIdle;
	timer_ = 0.0f;
	nextSceneName_.clear();
}

void CrtTransition::StartTransition(const std::string& nextScene, PostProcessRenderer::PostProcessMode postModeAfter) {
	if (state_ != State::kIdle) {
		return; // 既に遷移中なら無視
	}

	nextSceneName_ = nextScene;
	postModeAfter_ = postModeAfter;
	state_ = State::kGlitchBurst;
	timer_ = 0.0f;

	// ポストプロセスをCRTトランジションモードに切り替え
	PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kCrtTransition);
}

void CrtTransition::Update() {
	if (state_ == State::kIdle) {
		return;
	}

	const float dt = 0.016f;
	timer_ += dt;

	auto* crtEffect = PostProcessRenderer::GetInstance()->GetEffect<CrtTransitionEffect>(
		PostProcessRenderer::PostProcessMode::kCrtTransition);
	if (!crtEffect) {
		state_ = State::kIdle;
		return;
	}

	switch (state_) {
	case State::kGlitchBurst: {
		// フェーズ1: 激しいRGBグリッチ（クレッシェンドで時間経過に伴い激化）
		const float duration = (std::max)(glitchDuration_, 0.05f);
		float progress = (std::clamp)(timer_ / duration, 0.0f, 1.0f);

		float curve = std::pow(progress, 1.3f);
		float glitchIntensity = 0.35f + curve * 0.65f;
		float chromaticAberration = 0.010f + curve * 0.060f;
		float scanline = 0.20f + curve * 0.40f;

		crtEffect->SetGlitchIntensity(glitchIntensity);
		crtEffect->SetChromaticAberration(chromaticAberration);
		crtEffect->SetScanlineIntensity(scanline);
		crtEffect->SetCollapseX(1.0f);
		crtEffect->SetCollapseY(1.0f);
		crtEffect->SetFlashIntensity(0.0f);
		crtEffect->SetScreenCurvature(0.04f);

		if (timer_ >= duration) {
			state_ = State::kShutdownY;
			timer_ = 0.0f;
		}
		break;
	}

	case State::kShutdownY: {
		// フェーズ2: 上下から中央へ急激に潰れ、白熱水平ラインへ
		const float duration = (std::max)(shutdownYDuration_, 0.02f);
		float progress = (std::clamp)(timer_ / duration, 0.0f, 1.0f);

		// 急速にYスケールが縮小 (3.5乗イージング)
		float scaleY = std::pow(1.0f - progress, 3.5f);
		scaleY = (std::max)(scaleY, 0.003f);

		crtEffect->SetCollapseX(1.0f);
		crtEffect->SetCollapseY(scaleY);
		// 潰れるほどエネルギーが集中して白熱発光
		crtEffect->SetFlashIntensity(progress * 1.3f);
		crtEffect->SetGlitchIntensity(0.4f * (1.0f - progress));
		crtEffect->SetChromaticAberration(0.04f);
		crtEffect->SetScanlineIntensity(0.2f);
		crtEffect->SetScreenCurvature(0.04f);

		if (timer_ >= duration) {
			state_ = State::kShutdownX;
			timer_ = 0.0f;
		}
		break;
	}

	case State::kShutdownX: {
		// フェーズ3: 水平ラインが左右から中央へ収縮してドットへ
		const float duration = (std::max)(shutdownXDuration_, 0.02f);
		float progress = (std::clamp)(timer_ / duration, 0.0f, 1.0f);

		// 水平ラインが中央のドットへ収縮 (2.5乗イージング)
		float scaleX = std::pow(1.0f - progress, 2.5f);
		scaleX = (std::max)(scaleX, 0.002f);

		crtEffect->SetCollapseX(scaleX);
		crtEffect->SetCollapseY(0.003f);
		// 中心の点に光が凝縮
		crtEffect->SetFlashIntensity(1.3f + progress * 0.3f);
		crtEffect->SetGlitchIntensity(0.0f);
		crtEffect->SetChromaticAberration(0.02f);
		crtEffect->SetScanlineIntensity(0.0f);
		crtEffect->SetScreenCurvature(0.04f);

		if (timer_ >= duration) {
			state_ = State::kBlackout;
			timer_ = 0.0f;
		}
		break;
	}

	case State::kBlackout: {
		// フェーズ4: 中心の輝点がプツンと消えて暗転、その後完全な暗転の余韻を待ってからシーン切替
		constexpr float kDotFadeDuration = 0.04f; // 輝点がプツンと消える時間
		const float totalDuration = kDotFadeDuration + (std::max)(blackoutDuration_, 0.0f);

		crtEffect->SetCollapseX(0.0f);
		crtEffect->SetCollapseY(0.0f);
		crtEffect->SetGlitchIntensity(0.0f);
		crtEffect->SetChromaticAberration(0.0f);

		if (timer_ < kDotFadeDuration) {
			// プツンと消える残光
			float fadeProgress = timer_ / kDotFadeDuration;
			crtEffect->SetFlashIntensity((1.0f - fadeProgress) * 1.6f);
		} else {
			// 完全暗転（真っ暗な静寂）
			crtEffect->SetFlashIntensity(0.0f);
		}

		if (timer_ >= totalDuration) {
			crtEffect->SetFlashIntensity(0.0f);
			// 暗転待機が完了した瞬間にシーンを切り替え！
			if (!nextSceneName_.empty()) {
				SceneManager::GetInstance()->ChangeScene(nextSceneName_);
				nextSceneName_.clear();
			}

			// 新しいシーンでの再点灯（Turn-On）へ移行
			state_ = State::kTurnOnDot;
			timer_ = 0.0f;
		}
		break;
	}

	case State::kTurnOnDot: {
		// フェーズ5: 新シーンで中心の輝点がパッと点灯
		const float duration = (std::max)(turnOnDotDuration_, 0.01f);

		crtEffect->SetCollapseX(0.002f);
		crtEffect->SetCollapseY(0.003f);
		crtEffect->SetFlashIntensity(1.5f);
		crtEffect->SetGlitchIntensity(0.0f);
		crtEffect->SetChromaticAberration(0.01f);
		crtEffect->SetScanlineIntensity(0.2f);
		crtEffect->SetScreenCurvature(0.04f);

		if (timer_ >= duration) {
			state_ = State::kTurnOnX;
			timer_ = 0.0f;
		}
		break;
	}

	case State::kTurnOnX: {
		// フェーズ6: 水平に一気に展開
		const float duration = (std::max)(turnOnXDuration_, 0.02f);
		float progress = (std::clamp)(timer_ / duration, 0.0f, 1.0f);

		// 急速展開 (0.4乗)
		float scaleX = std::pow(progress, 0.4f);
		scaleX = (std::clamp)(scaleX, 0.002f, 1.0f);

		crtEffect->SetCollapseX(scaleX);
		crtEffect->SetCollapseY(0.004f);
		crtEffect->SetFlashIntensity(1.5f - progress * 0.5f);
		crtEffect->SetGlitchIntensity(0.0f);
		crtEffect->SetChromaticAberration(0.015f * (1.0f - progress));

		if (timer_ >= duration) {
			state_ = State::kTurnOnY;
			timer_ = 0.0f;
		}
		break;
	}

	case State::kTurnOnY: {
		// フェーズ7: 上下に一気に展開して全画面へ
		const float duration = (std::max)(turnOnYDuration_, 0.02f);
		float progress = (std::clamp)(timer_ / duration, 0.0f, 1.0f);

		// 急速展開 (0.4乗)
		float scaleY = std::pow(progress, 0.4f);
		scaleY = (std::clamp)(scaleY, 0.004f, 1.0f);

		crtEffect->SetCollapseX(1.0f);
		crtEffect->SetCollapseY(scaleY);
		// 光が引いて通常の明るさへ
		crtEffect->SetFlashIntensity((1.0f - progress) * 1.0f);
		crtEffect->SetGlitchIntensity(0.0f);
		crtEffect->SetChromaticAberration(0.0f);

		if (timer_ >= duration) {
			// トランジション完了！
			state_ = State::kIdle;
			timer_ = 0.0f;
			PostProcessRenderer::GetInstance()->SetMode(postModeAfter_);
		}
		break;
	}

	default:
		break;
	}
}

void CrtTransition::UpdateImGui() {
#ifdef USE_IMGUI
	if (ImGui::CollapsingHeader("CRT Scene Transition Manager", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("Glitch Duration", &glitchDuration_, 0.2f, 2.0f, "%.2f s");
		ImGui::SliderFloat("Blackout Wait", &blackoutDuration_, 0.0f, 1.5f, "%.2f s");

		const char* stateStr = "Idle";
		switch (state_) {
		case State::kGlitchBurst: stateStr = ">> GLITCH BURST <<"; break;
		case State::kShutdownY:   stateStr = "Vertical Collapse"; break;
		case State::kShutdownX:   stateStr = "Horizontal Collapse"; break;
		case State::kBlackout:    stateStr = "Blackout (Scene Switching)"; break;
		case State::kTurnOnDot:   stateStr = "Turn-on Dot"; break;
		case State::kTurnOnX:     stateStr = "Turn-on Horizontal Expand"; break;
		case State::kTurnOnY:     stateStr = "Turn-on Vertical Expand"; break;
		default: break;
		}
		ImGui::Text("State: %s", stateStr);
	}
#endif
}
