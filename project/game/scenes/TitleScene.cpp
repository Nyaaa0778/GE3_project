#include "TitleScene.h"

#include <MyEngine.h>
#include "LightManager.h"
#include <numbers>

#include "Skybox.h"
#include "DebugCamera.h"
#include "Plane.h"
#include "TextureManager.h"
#include "WireframeObject.h"
#include "CityBackground.h"
#include "Sprite.h"
#include "PostProcessRenderer.h"
#include "PostProcessEffects.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

TitleScene::TitleScene() = default;
TitleScene::~TitleScene() = default;

void TitleScene::Initialize() {
	// 0. 最背面用の黒スプライト初期化（クリアカラーを変更せず背景を黒にする）
	backgroundSprite_ = std::make_unique<Sprite>();
	backgroundSprite_->Initialize("white.png", {0.0f, 0.0f}, {0.0f, 0.0f});
	backgroundSprite_->SetSize({1280.0f, 720.0f});
	backgroundSprite_->SetColor({0.0f, 0.0f, 0.0f, 1.0f}); // 完全な黒

	// 1. 通常カメラの初期化（サイバーハイウェイと摩天楼を前方に見渡すアングル）
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({0.0f, 0.0f, 0.0f});
	camera_->SetTranslate({0.0f, 3.5f, -8.0f});
	camera_->CreateConstantBuffer();

	// 2. デバッグカメラの初期化
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize();
	debugCamera_->SetRotate(camera_->GetRotate());
	debugCamera_->SetTranslate(camera_->GetTranslate());
	debugCamera_->CalculateMatrix();
	debugCamera_->CreateConstantBuffer();

	// 3. サイバーパンク背景都市の初期化
	cityBackground_ = std::make_unique<CityBackground>();
	cityBackground_->Initialize(camera_.get());
	cityBackground_->SetScrollSpeed(0.0f); // デフォルトは静止配置（チラつき防止）

	// 5. グリッチ・ポストプロセスの有効化
	PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kGlitch);
}

void TitleScene::Update() {
	auto input = Input::GetInstance();

	if (useDebugCamera_) {
		debugCamera_->Update(camera_.get());
		camera_->CalculateMatrix();
	} else {
		camera_->CalculateMatrix();
	}

	// 背景黒スプライトの更新
	if (backgroundSprite_) {
		backgroundSprite_->Update();
	}

	// 背景都市の更新（カメラ追従＋スクロール＆ネオン明滅）
	if (cityBackground_) {
		cityBackground_->SetCamera(camera_.get());
		cityBackground_->Update();
	}

	// グリッチエフェクトパラメータの反映
	if (auto* glitch = PostProcessRenderer::GetInstance()->GetEffect<GlitchEffect>(PostProcessRenderer::PostProcessMode::kGlitch)) {
		glitch->SetIntensity(enableGlitch_ ? glitchIntensity_ : 0.0f);
		glitch->SetChromaticAberration(enableGlitch_ ? chromaticAberration_ : 0.0f);
		glitch->SetScanlineIntensity(enableGlitch_ ? scanlineIntensity_ : 0.0f);
		glitch->SetSpeed(glitchSpeed_);
		glitch->SetFrequency(glitchFrequency_);
		glitch->SetBlockCount(glitchBlockCount_);
		glitch->SetShiftScale(glitchShiftScale_);
	}

	UpdateImGui();

	// --- シーン遷移判定 (Space / Aボタン) ---
	if (input->TriggerKey(DIK_SPACE) || input->TriggerButton(XINPUT_GAMEPAD_A)) {
		input->SetShake(0.0f, 0.0f);
		SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
		return;
	}
}

void TitleScene::Draw() {
	// 0. 最背面の黒スプライト（青いクリア色を黒で覆う）
	if (backgroundSprite_) {
		backgroundSprite_->Draw();
	}

	// 1. サイバーパンク背景都市（地面グリッド＋摩天楼＋ネオンサン）
	if (cityBackground_) {
		cityBackground_->Draw();
	}
}

void TitleScene::Finalize() {
	// 次のシーンのためにポストプロセスを通常に戻す
	PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kNormal);

	backgroundSprite_.reset();
	cityBackground_.reset();
}

void TitleScene::UpdateImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Cyberpunk Title Settings");

	ImGui::Checkbox("Use Debug Camera", &useDebugCamera_);

	if (ImGui::CollapsingHeader("City Background", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (cityBackground_) {
			float speed = cityBackground_->GetScrollSpeed();
			if (ImGui::SliderFloat("Scroll Speed", &speed, 0.0f, 30.0f, "%.1f")) {
				cityBackground_->SetScrollSpeed(speed);
			}

			float baseY = cityBackground_->GetBaseY();
			if (ImGui::SliderFloat("Base Y (Height)", &baseY, -30.0f, 10.0f, "%.1f")) {
				cityBackground_->SetBaseY(baseY);
			}

			static bool pulse = true;
			if (ImGui::Checkbox("Neon Pulse & Flicker", &pulse)) {
				cityBackground_->SetPulseEnabled(pulse);
			}
		}
	}

	if (ImGui::CollapsingHeader("Glitch PostProcess", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Enable Glitch", &enableGlitch_);
		if (enableGlitch_) {
			ImGui::SliderFloat("Glitch Intensity", &glitchIntensity_, 0.0f, 1.0f);
			ImGui::SliderFloat("Chromatic Aberration", &chromaticAberration_, 0.0f, 0.03f, "%.4f");
			ImGui::SliderFloat("Scanline Intensity", &scanlineIntensity_, 0.0f, 1.0f);
			ImGui::SliderFloat("Rhythm (Speed)", &glitchSpeed_, 1.0f, 60.0f, "%.1f steps/s");
			ImGui::SliderFloat("Frequency (Prob)", &glitchFrequency_, 0.0f, 1.0f, "%.2f");
			ImGui::SliderFloat("Block Count", &glitchBlockCount_, 5.0f, 120.0f, "%.0f bands");
			ImGui::SliderFloat("Shift Scale", &glitchShiftScale_, 0.0f, 3.0f, "%.2fx");
		}
	}

	ImGui::End();
#endif
}
