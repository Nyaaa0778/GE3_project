#include "TitleScene.h"

#include <MyEngine.h>

#include "DebugCamera.h"
#include "CityBackground.h"
#include "TitleLogo.h"
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

	// 1. 通常カメラの初期化（サイバーシティを上空から見下ろす俯瞰オービットカメラ）
	camera_ = std::make_unique<Camera>();
	camera_->SetFarClip(600.0f); // 奥行きを出すためファークリップを大幅拡張 (100m -> 600m)
	camera_->CreateConstantBuffer();
	UpdateOrbitCamera(); // オービットカメラの初期位置・角度を反映
	camera_->CalculateMatrix();

	// 2. デバッグカメラの初期化
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize();
	debugCamera_->SetRotate(camera_->GetRotate());
	debugCamera_->SetTranslate(camera_->GetTranslate());
	debugCamera_->SetFarClip(600.0f);
	debugCamera_->CalculateMatrix();
	debugCamera_->CreateConstantBuffer();

	// 3. サイバーパンク背景都市の初期化 (奥行き320mまでビルを配置、グリッド全長400m、80m〜300mで徐々にフェードアウト消滅)
	cityBackground_ = std::make_unique<CityBackground>();
	cityBackground_->Initialize(camera_.get(), 4.0f, 320.0f, 65, 8.0f, 200.0f, 80.0f, 300.0f);
	cityBackground_->SetScrollSpeed(0.0f); // デフォルトは静止配置（チラつき防止）
	cityBackground_->SetBuildAnimationEnabled(true); // 建物を徐々に枝分かれ形成

	// 4. タイトルロゴ「CYBERAIL」の初期化（都市と同じワイヤー形成演出＋視認性確保の半透明バックプレート）
	titleLogo_ = std::make_unique<TitleLogo>();
	titleLogo_->Initialize(); // nullptrで専用HUD正面カメラを生成

	// 5. グリッチ・ポストプロセスの有効化
	PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kGlitch);
}

void TitleScene::Update() {
	auto input = Input::GetInstance();

	if (useDebugCamera_) {
		debugCamera_->Update(camera_.get());
		camera_->CalculateMatrix();
	} else {
		// 俯瞰オービットカメラの自動旋回
		if (enableOrbitCamera_) {
			UpdateOrbitCamera();
		}
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

	// タイトルロゴ「CYBERAIL」の更新（文字形成アニメーション＆ネオン点灯）
	if (titleLogo_) {
		titleLogo_->Update();
	}

	// 間欠的グリッチ演出の制御（普段は静止・平穏、2.5〜5.5秒に1度だけ一瞬0.05〜0.10秒激しく乱れる）
	float activeIntensity = 0.0f;
	float activeAberration = 0.0f;

	if (enableGlitch_) {
		const float dt = 0.016f;
		if (!isGlitching_) {
			glitchTimer_ += dt;
			if (glitchTimer_ >= glitchNextInterval_) {
				isGlitching_ = true;
				glitchBurstTimer_ = 0.0f;
				// 一瞬の持続時間: 0.05秒〜0.10秒 (数フレーム程度)
				glitchBurstDuration_ = 0.05f + (static_cast<float>(rand()) / RAND_MAX) * 0.05f;
				// 約20%の確率で二連撃
				if ((static_cast<float>(rand()) / RAND_MAX) < 0.20f && glitchSubBurstCount_ == 0) {
					glitchSubBurstCount_ = 1;
				} else {
					glitchSubBurstCount_ = 0;
				}
			}
		} else {
			glitchBurstTimer_ += dt;
			activeIntensity = glitchIntensity_;
			activeAberration = chromaticAberration_;

			if (glitchBurstTimer_ >= glitchBurstDuration_) {
				isGlitching_ = false;
				glitchTimer_ = 0.0f;
				if (glitchSubBurstCount_ > 0) {
					// 二連撃の第2波は0.06秒〜0.12秒後の直後に発生
					glitchNextInterval_ = 0.06f + (static_cast<float>(rand()) / RAND_MAX) * 0.06f;
					glitchSubBurstCount_ = 0;
				} else {
					// 通常の静寂インターバル (2.5秒〜5.5秒)
					glitchNextInterval_ = 2.5f + (static_cast<float>(rand()) / RAND_MAX) * 3.0f;
				}
			}
		}
	}

	// グリッチエフェクトパラメータの反映
	if (auto* glitch = PostProcessRenderer::GetInstance()->GetEffect<GlitchEffect>(PostProcessRenderer::PostProcessMode::kGlitch)) {
		glitch->SetIntensity(activeIntensity);
		glitch->SetChromaticAberration(activeAberration);
		glitch->SetScanlineIntensity(scanlineIntensity_);
		glitch->SetSpeed(30.0f);
		glitch->SetFrequency(1.0f);
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

	// 2. タイトルロゴ「CYBERAIL」（半透明ダーク遮蔽プレート＋動的形成ワイヤー文字＋装飾枠）
	if (titleLogo_) {
		titleLogo_->Draw();
	}
}

void TitleScene::Finalize() {
	// 次のシーンのためにポストプロセスを通常に戻す
	PostProcessRenderer::GetInstance()->SetMode(PostProcessRenderer::PostProcessMode::kNormal);

	backgroundSprite_.reset();
	cityBackground_.reset();
	titleLogo_.reset();
}

void TitleScene::UpdateOrbitCamera() {
	if (!camera_) return;

	const float dt = 0.016f;
	orbitAngle_ += orbitSpeed_ * dt;
	// 角度の正規化 (0〜2pi)
	const float kTwoPi = 6.2831853f;
	if (orbitAngle_ > kTwoPi) {
		orbitAngle_ -= kTwoPi;
	} else if (orbitAngle_ < -kTwoPi) {
		orbitAngle_ += kTwoPi;
	}

	// 俯瞰オービット位置 (ある一点 orbitCenter_ を中心とする円周上)
	float camX = orbitCenter_.x + orbitRadius_ * std::sin(orbitAngle_);
	float camZ = orbitCenter_.z - orbitRadius_ * std::cos(orbitAngle_);
	float camY = orbitCenter_.y + orbitHeight_;
	Vector3 camPos = {camX, camY, camZ};

	// 中心点に向かう方向ベクトル (Camera -> Center)
	Vector3 dir = {
		orbitCenter_.x - camPos.x,
		orbitCenter_.y - camPos.y,
		orbitCenter_.z - camPos.z
	};

	float distXZ = std::sqrt(dir.x * dir.x + dir.z * dir.z);
	float yaw = std::atan2(dir.x, dir.z);
	float pitch = std::atan2(-dir.y, distXZ); // dir.y < 0 のため -dir.y > 0 (俯瞰見下ろし角)

	camera_->SetTranslate(camPos);
	camera_->SetRotate({pitch, yaw, 0.0f});
}

void TitleScene::UpdateImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Cyberpunk Title Settings");

	ImGui::Checkbox("Use Debug Camera", &useDebugCamera_);

	if (ImGui::CollapsingHeader("Cinematic Orbit Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Enable Orbit", &enableOrbitCamera_);
		if (enableOrbitCamera_) {
			ImGui::SliderFloat3("Center Point", &orbitCenter_.x, -50.0f, 150.0f);
			ImGui::SliderFloat("Orbit Radius", &orbitRadius_, 10.0f, 150.0f, "%.1f m");
			ImGui::SliderFloat("Height (Look Down)", &orbitHeight_, 0.0f, 80.0f, "%.1f m");
			ImGui::SliderFloat("Orbit Speed", &orbitSpeed_, -0.5f, 0.5f, "%.3f rad/s");
			ImGui::SliderAngle("Orbit Angle", &orbitAngle_);
		}
	}

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

			bool buildAnim = cityBackground_->IsBuildAnimationEnabled();
			if (ImGui::Checkbox("Building Formation Animation", &buildAnim)) {
				cityBackground_->SetBuildAnimationEnabled(buildAnim);
			}
			if (buildAnim) {
				ImGui::SameLine();
				if (ImGui::Button("Replay Formation")) {
					cityBackground_->ResetBuildAnimation();
				}
			}

			float fogNear = cityBackground_->GetFogNear();
			float fogFar = cityBackground_->GetFogFar();
			bool fogChanged = false;
			if (ImGui::SliderFloat("Depth Fade Near (Start)", &fogNear, 10.0f, 300.0f, "%.1f m")) {
				fogChanged = true;
			}
			if (ImGui::SliderFloat("Depth Fade Far (End)", &fogFar, 50.0f, 600.0f, "%.1f m")) {
				fogChanged = true;
			}
			if (fogChanged) {
				cityBackground_->SetFog(fogNear, fogFar);
			}

			if (camera_) {
				float farClip = camera_->GetFarClip();
				if (ImGui::SliderFloat("Camera Far Clip", &farClip, 100.0f, 1000.0f, "%.0f m")) {
					camera_->SetFarClip(farClip);
					if (debugCamera_) {
						debugCamera_->SetFarClip(farClip);
					}
				}
			}
		}
	}

	if (ImGui::CollapsingHeader("Glitch PostProcess", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Enable Glitch", &enableGlitch_);
		if (enableGlitch_) {
			ImGui::SliderFloat("Burst Intensity", &glitchIntensity_, 0.0f, 1.0f);
			ImGui::SliderFloat("Chromatic Aberration", &chromaticAberration_, 0.0f, 0.03f, "%.4f");
			ImGui::SliderFloat("Scanline Intensity", &scanlineIntensity_, 0.0f, 0.5f);
			ImGui::SliderFloat("Block Count", &glitchBlockCount_, 5.0f, 120.0f, "%.0f bands");
			ImGui::SliderFloat("Shift Scale", &glitchShiftScale_, 0.0f, 3.0f, "%.2fx");

			ImGui::Separator();
			ImGui::Text("Status: %s", isGlitching_ ? ">> GLITCHING << (Active)" : "Calm (Idle)");
			if (ImGui::Button("Trigger Glitch Now")) {
				isGlitching_ = true;
				glitchBurstTimer_ = 0.0f;
				glitchBurstDuration_ = 0.08f;
			}
		}
	}

	if (ImGui::CollapsingHeader("Cyber Title Logo (CYBERAIL)", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (titleLogo_) {
			if (ImGui::Button("Replay Logo Formation")) {
				titleLogo_->ResetAnimation();
			}
			ImGui::SameLine();
			ImGui::Text("Status: %s", titleLogo_->IsAllBuilt() ? "Built (Pulsing)" : "Constructing...");

			static bool enablePlate = true;
			if (ImGui::Checkbox("Backdrop Dark Plate", &enablePlate)) {
				titleLogo_->SetBackdropEnabled(enablePlate);
			}

			static float plateAlpha = 0.75f;
			if (ImGui::SliderFloat("Backdrop Plate Alpha", &plateAlpha, 0.0f, 1.0f, "%.2f")) {
				titleLogo_->SetBackdropAlpha(plateAlpha);
			}

			Vector3 logoPos = titleLogo_->GetPosition();
			if (ImGui::SliderFloat3("Logo Position", &logoPos.x, -5.0f, 5.0f, "%.2f")) {
				titleLogo_->SetPosition(logoPos);
			}

			Vector3 logoScale = titleLogo_->GetScale();
			if (ImGui::SliderFloat("Logo Scale", &logoScale.x, 0.3f, 2.0f, "%.2f")) {
				logoScale.y = logoScale.x;
				logoScale.z = logoScale.x;
				titleLogo_->SetScale(logoScale);
			}

			Vector3 logoRot = titleLogo_->GetRotation();
			if (ImGui::SliderFloat3("Logo 3D Rotation", &logoRot.x, -0.6f, 0.6f, "%.2f rad")) {
				titleLogo_->SetRotation(logoRot);
			}

			bool hover = titleLogo_->IsHoverEnabled();
			if (ImGui::Checkbox("Enable Floating Hover", &hover)) {
				titleLogo_->SetHoverEnabled(hover);
			}

			Vector3 promptPos = titleLogo_->GetPromptPosition();
			if (ImGui::SliderFloat3("Prompt Position", &promptPos.x, -5.0f, 5.0f, "%.2f")) {
				titleLogo_->SetPromptPosition(promptPos);
			}
		}
	}

	ImGui::End();
#endif
}
