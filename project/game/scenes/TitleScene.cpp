#include "TitleScene.h"
#include "DevEditor.h"

#include <MyEngine.h>
#include "LightManager.h"
#include <numbers>

#include "Skybox.h"
#include "DebugCamera.h"
#include "Plane.h"
#include "PostProcessRenderer.h"
#include "TextureManager.h"

TitleScene::TitleScene() = default;
TitleScene::~TitleScene() = default;

void TitleScene::Initialize() {
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({0.5f, 0.0f, 0.0f});
	camera_->SetTranslate({0.0f, 8.0f, -15.0f});
	camera_->CreateConstantBuffer();

	// ② デバッグカメラの初期化（★追加）
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize(); // Inputの取得など
	// 通常カメラの初期位置に合わせる
	debugCamera_->SetRotate(camera_->GetRotate());
	debugCamera_->SetTranslate(camera_->GetTranslate());
	debugCamera_->CalculateMatrix();
	debugCamera_->CreateConstantBuffer();

	skybox_ = std::make_unique<Skybox>();
	//std::array<std::string, 6> skyboxPaths = {
	//	"resources/sprites/pink.png",  // +X
	//	"resources/sprites/red.png",   // -X
	//	"resources/sprites/blue.png",     // +Y
	//	"resources/sprites/green.png",   // -Y
	//	"resources/sprites/purple.png",  // +Z
	//	"resources/sprites/yellow.png",   // -Z
	//};

	//skybox_->Initialize(skyboxPaths, camera_.get());

	skybox_->Initialize("resources/sprites/rostock_laage_airport_4k.dds", camera_.get());

	// object3dの初期化
	sphere_ = std::make_unique<Object3d>();
	sphere_->Initialize("sphere");
	sphere_->SetCamera(camera_.get());
	sphere_->SetEnvironmentTextureHandle(skybox_->GetTextureSrvHandleGPU());

	//terrain_ = std::make_unique<Object3d>();
	//terrain_->Initialize("terrain");
	//terrain_->SetRotation({0.0f, -1.5708f, 0.0f});
	//terrain_->SetCamera(camera_.get());

	primitive_ = std::make_unique<Plane>();
	primitive_->Initialize("monsterBall.png");
	primitive_->SetCamera(camera_.get());
	// Plane へのキャストが必要な初期設定があればここで行うか、Plane内で完結させる
	if (auto* palne = dynamic_cast<Plane*>(primitive_.get())) {
		palne->SetPosition({ 2.0f, 0.0f, 0.0f }); // 少しずらしておく
	}

	// spriteの初期化
	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize("monsterBall.png");

	// --------------------------------------------------
	// ② ラジカセ（AudioSource）の実体を作る
	// --------------------------------------------------
	bgm_ = AudioManager::LoadAudio("title.mp3");
	se_ = AudioManager::LoadAudio("bgmGamePlay.wav", SoundGroup::SE);

	ParticleManager::GetInstance()->CreateParticleGroup("CircleParticle", "resources/sprites/circle.png", ParticleManager::ParticleShape::kPlane);

	// 2. エミッタの生成（グループ名、追従するTransformのポインタ、発生間隔、1回の数）
	// titleTransform_ はタイトルロゴや背景の座標を指す想定
	emitter_ = std::make_unique<ParticleEmitter>("CircleParticle", &particleTransform_, 0.2f, 3);

	// ノイズテクスチャを事前にロードしてキャッシュしておく
	TextureManager::GetInstance()->LoadTexture("resources/sprites/noise0.png");
	TextureManager::GetInstance()->LoadTexture("resources/sprites/noise1.png");

	// ポストプロセスの初期設定 (ディゾルブモードを確定し、ロード済みテクスチャを設定)
	auto postProcess = PostProcessRenderer::GetInstance();
	postProcess->SetMode(PostProcessRenderer::PostProcessMode::kDissolve);
	postProcess->SetDissolveNoiseTexture("resources/sprites/noise0.png");
	postProcess->SetDissolveThreshold(0.0f);
}

void TitleScene::Update() {
	auto input = Input::GetInstance();

#ifdef USE_IMGUI
	UpdateImGui();

	// エディタモード状態をデバッグカメラのデフォルト状態と連携
	bool isEditor = DevEditor::GetInstance()->IsEditorMode();
	if (isEditor) {
		useDebugCamera_ = true;
	}

	bool isPaused = DevEditor::GetInstance()->IsPaused();
	if (DevEditor::GetInstance()->IsStepRequested()) {
		isPaused = false;
		DevEditor::GetInstance()->ClearStepRequest();
	}
#else
	bool isPaused = false;
#endif

	if (useDebugCamera_) {
		// ★ debugCameraController_ に camera_ を「操作してくれ」と頼む
		debugCamera_->Update(camera_.get());
	} else {
		// 通常時のカメラ挙動（固定やパス移動など）
		camera_->CalculateMatrix();
	}

	if (!isPaused) {
		if (emitter_) {
			emitter_->Update();
		}

		ParticleManager::GetInstance()->Update(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());

		// 【効果音のテスト】スペースキーを押したら「決定音」を鳴らす
		if (input->TriggerKey(DIK_SPACE)) {
			AudioManager::PlayAudio(se_); // 何も書かない、または false で1回だけ再生
			isDissolving_ = true;
			isFadingOut_ = !isFadingOut_;
		}

		// ディゾルブのアニメーション更新
		if (isDissolving_) {
			if (isFadingOut_) {
				dissolveThreshold_ += dissolveSpeed_;
				if (dissolveThreshold_ >= 1.0f) {
					dissolveThreshold_ = 1.0f;
					isDissolving_ = false;
				}
			} else {
				dissolveThreshold_ -= dissolveSpeed_;
				if (dissolveThreshold_ <= 0.0f) {
					dissolveThreshold_ = 0.0f;
					isDissolving_ = false;
				}
			}
			PostProcessRenderer::GetInstance()->SetDissolveThreshold(dissolveThreshold_);
		}

		// 【停止のテスト】Bキーを押したらBGMだけをピタッと止める
		if (input->TriggerKey(DIK_B)) {
			AudioManager::StopAudio(bgm_);
		}

		// --- 1. シーン遷移判定 ---
		if (input->TriggerKey(DIK_RETURN) || input->TriggerButton(XINPUT_GAMEPAD_A)) {
			// 遷移時に振動を止める（重要）
			input->SetShake(0.0f, 0.0f);
			SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
			return;
		}

		// --- 2. 押し込み判定と振動の処理 ---
		if (input->PushButton(XINPUT_GAMEPAD_LEFT_THUMB)) {
			// 振動させる
			input->SetShake(0.3f, 0.7f);
		} else {
			// 離していたら振動を止める
			input->SetShake(0.0f, 0.0f);
		}

		if (input->TriggerButton(XINPUT_GAMEPAD_RIGHT_THUMB)) {
			input->SetShake(1.0f, 1.0f, 3.0f); // 1秒間ドカン！と震えて勝手に止まる
		}
	}

	// 常にオブジェクトの行列更新は行ってGPUに送る（一時停止中にインスペクターで編集可能にするため）
	sphere_->Update();
	primitive_->Update();
	sprite_->Update();
}

void TitleScene::Draw() {

	sphere_->Draw();
	//terrain_->Draw();

	primitive_->Draw();
	//skybox_->Draw();

	// パーティクルの描画（インスタンシング描画が実行される）
	ParticleManager::GetInstance()->Draw();

	//sprite_->Draw();
}

void TitleScene::Finalize() {

}

void TitleScene::UpdateImGui() {
#ifdef USE_IMGUI
	if (DevEditor::GetInstance()->IsEditorMode()) {
		return;
	}

	// 1つの大きなウィンドウとしてサイズを設定（初回のみ適用、リサイズ可能）
	ImGui::SetNextWindowSize(ImVec2(450.0f, 600.0f), ImGuiCond_Once);

	// すべてを内包する1つのウィンドウを開始
	if (ImGui::Begin("デバッグメニュー")) {
		DrawSceneInspector();
	}
	ImGui::End(); // メインウィンドウの終了
#endif
}

void TitleScene::DrawSceneInspector() {
#ifdef USE_IMGUI
	// ──────────────────────────────────────────────────
	// 1. Sprite Object
	// ──────────────────────────────────────────────────
	if (ImGui::CollapsingHeader("スプライトオブジェクト")) {
		// 位置
		{
			Vector2 pos = sprite_->GetPosition();
			if (ImGui::DragFloat2("座標", &pos.x, 1.0f, 0.0f, 0.0f, "%.1f")) {
				sprite_->SetPosition(pos);
			}
		}

		// スケール
		{
			Vector2 scale = sprite_->GetScale();
			if (ImGui::DragFloat2("スケール", &scale.x, 0.1f, -10.0f, 10.0f, "%.1f")) {
				sprite_->SetScale(scale);
			}
		}

		// 回転
		{
			float rot = sprite_->GetRotate();
			if (ImGui::DragFloat("回転", &rot, 0.1f, -6.28f, 6.28f, "%.1f")) {
				sprite_->SetRotation(rot);
			}
		}

		// 色・透明度
		{
			Vector4 color = sprite_->GetColor();
			float col[4] = {color.x, color.y, color.z, color.w};
			if (ImGui::ColorEdit4("色", col)) {
				Vector4 newColor(col[0], col[1], col[2], col[3]);
				sprite_->SetColor(newColor);
			}
		}
	}

	// ──────────────────────────────────────────────────
	// 2. Camera Control
	// ──────────────────────────────────────────────────
	if (ImGui::CollapsingHeader("カメラコントロール")) {
		Vector3 camPos = camera_->GetTranslate();
		if (ImGui::DragFloat3("座標", &camPos.x, 0.1f)) {
			camera_->SetTranslate(camPos);
		}

		Vector3 camRot = camera_->GetRotate();
		if (ImGui::DragFloat3("回転", &camRot.x, 0.01f)) {
			camera_->SetRotate(camRot);
		}

		if (ImGui::Button("カメラリセット")) {
			camera_->SetTranslate({0.0f, 0.0f, -10.0f});
			camera_->SetRotate({0.0f, 0.0f, 0.0f});
		}
	}

	// ──────────────────────────────────────────────────
	// 3. PostProcess Settings (★追加分)
	// ──────────────────────────────────────────────────
	if (ImGui::CollapsingHeader("ポストプロセス設定")) {
		static int currentMode = static_cast<int>(PostProcessRenderer::GetInstance()->GetMode());
		const char* modes[] = {"通常", "ラジアルブラー", "ボックスフィルタ", "ガウシアンフィルタ", "グレースケール", "アウトライン", "ビネット", "ディゾルブ", "ランダムノイズ"};
		if (ImGui::Combo("描画モード", &currentMode, modes, IM_ARRAYSIZE(modes))) {
			PostProcessRenderer::GetInstance()->SetMode(static_cast<PostProcessRenderer::PostProcessMode>(currentMode));
		}

		if (currentMode == static_cast<int>(PostProcessRenderer::PostProcessMode::kVignetting)) {
			Vector4 color = PostProcessRenderer::GetInstance()->GetVignetteColor();
			float c[4] = {color.x, color.y, color.z, color.w};
			if (ImGui::ColorEdit4("ビネットカラー", c)) {
				PostProcessRenderer::GetInstance()->SetVignetteColor({c[0], c[1], c[2], c[3]});
			}
		}

		if (currentMode == static_cast<int>(PostProcessRenderer::PostProcessMode::kDissolve)) {
			float threshold = PostProcessRenderer::GetInstance()->GetDissolveThreshold();
			if (ImGui::SliderFloat("ディゾルブしきい値", &threshold, 0.0f, 1.0f)) {
				PostProcessRenderer::GetInstance()->SetDissolveThreshold(threshold);
			}

			static int noiseIndex = 0;
			const char* noises[] = {"ノイズ0", "ノイズ1"};
			if (ImGui::Combo("ディゾルブノイズ", &noiseIndex, noises, IM_ARRAYSIZE(noises))) {
				if (noiseIndex == 0) {
					PostProcessRenderer::GetInstance()->SetDissolveNoiseTexture("resources/sprites/noise0.png");
				} else {
					PostProcessRenderer::GetInstance()->SetDissolveNoiseTexture("resources/sprites/noise1.png");
				}
			}
		}
	}

	// ──────────────────────────────────────────────────
	// 4. Particle Debug
	// ──────────────────────────────────────────────────
	if (ImGui::CollapsingHeader("パーティクルデバッグ")) {
		auto& groups = ParticleManager::GetInstance()->GetGroups();
		for (auto& [name, group] : groups) {
			ImGui::Checkbox((name + " ビルボード").c_str(), &group.useBillboard);
		}
	}

	// ──────────────────────────────────────────────────
	// 5. Primitive Settings
	// ──────────────────────────────────────────────────
	primitive_->DrawImGui("プリミティブ設定");

	// ──────────────────────────────────────────────────
	// 6. Object Settings
	// ──────────────────────────────────────────────────
	if (ImGui::CollapsingHeader("オブジェクト設定")) {
		Vector3 rotate = sphere_->GetRotate();
		if (ImGui::DragFloat3("球体回転", &rotate.x, 0.01f, 0.1f, 100.0f)) {
			sphere_->SetRotation(rotate);
		}

		float envCoeff = sphere_->GetEnvironmentCoefficient();
		if (ImGui::SliderFloat("反射強度", &envCoeff, 0.0f, 1.0f)) {
			sphere_->SetEnvironmentCoefficient(envCoeff);
		}
	}

	// ──────────────────────────────────────────────────
	// 7. Debug Console
	// ──────────────────────────────────────────────────
	if (ImGui::CollapsingHeader("デバッグコンソール")) {
		if (ImGui::Checkbox("デバッグカメラを使用", &useDebugCamera_)) {
			if (useDebugCamera_) {
				debugCamera_->SetRotate(camera_->GetRotate());
				debugCamera_->SetTranslate(camera_->GetTranslate());
				debugCamera_->CalculateMatrix();
			}
		}
		// ... 略 ...
	}

	// ──────────────────────────────────────────────────
	// 8. Light Settings (元々コメントアウトされていた部分も統合)
	// ──────────────────────────────────────────────────
	/*
	if (ImGui::CollapsingHeader("Light Settings")) {
		LightManager* lightManager = LightManager::GetInstance();

		if (ImGui::TreeNode("Directional Light")) {
			Vector4 color = lightManager->GetDirectionalLightColor();
			Vector3 direction = lightManager->GetDirectionalLightDirection();
			float intensity = lightManager->GetDirectionalLightIntensity();

			ImGui::ColorEdit4("Color", &color.x);
			ImGui::DragFloat3("Direction", &direction.x, 0.01f, -1.0f, 1.0f);
			ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 10.0f);

			lightManager->SetDirectionalLightColor(color);
			lightManager->SetDirectionalLightDirection(direction);
			lightManager->SetDirectionalLightIntensity(intensity);

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Local Light")) {
			LocalLightType currentType = lightManager->GetLocalLightType();
			Vector4 color = lightManager->GetLocalLightColor();
			Vector3 position = lightManager->GetLocalLightPosition();
			float intensity = lightManager->GetLocalLightIntensity();
			float distance = lightManager->GetLocalLightDistance();
			float decay = lightManager->GetLocalLightDecay();

			const char* localLightTypeNames[] = {"Point", "Spot"};
			int typeIndex = static_cast<int>(currentType);
			if (ImGui::Combo("Light Type", &typeIndex, localLightTypeNames, IM_ARRAYSIZE(localLightTypeNames))) {
				lightManager->SetLocalLightType(static_cast<LocalLightType>(typeIndex));
			}

			ImGui::Separator();

			ImGui::ColorEdit4("Color", &color.x);
			ImGui::DragFloat3("Position", &position.x, 0.1f);
			ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100.0f);
			ImGui::DragFloat("Distance", &distance, 0.1f, 0.0f, 100.0f);
			ImGui::DragFloat("Decay", &decay, 0.01f, 0.0f, 10.0f);

			lightManager->SetLocalLightColor(color);
			lightManager->SetLocalLightPosition(position);
			lightManager->SetLocalLightIntensity(intensity);
			lightManager->SetLocalLightDistance(distance);
			lightManager->SetLocalLightDecay(decay);

			if (currentType == LocalLightType::kSpot) {
				ImGui::Text("Spotlight Settings");

				Vector3 direction = lightManager->GetLocalLightDirection();
				float cosAngle = lightManager->GetLocalLightCosAngle();
				float cosFalloff = lightManager->GetLocalLightCosFalloffStart();

				float angleDeg = std::acos(cosAngle) * 180.0f / std::numbers::pi_v<float>;
				float falloffDeg = std::acos(cosFalloff) * 180.0f / std::numbers::pi_v<float>;

				ImGui::DragFloat3("Direction", &direction.x, 0.01f, -1.0f, 1.0f);
				if (ImGui::DragFloat("Angle", &angleDeg, 0.1f, 0.0f, 90.0f)) {
					cosAngle = std::cos(angleDeg * std::numbers::pi_v<float> / 180.0f);
				}
				if (ImGui::DragFloat("Falloff Start", &falloffDeg, 0.1f, 0.0f, angleDeg)) {
					cosFalloff = std::cos(falloffDeg * std::numbers::pi_v<float> / 180.0f);
				}

				lightManager->SetLocalLightDirection(direction);
				lightManager->SetLocalLightCosAngle(cosAngle);
				lightManager->SetLocalLightCosFalloffStart(cosFalloff);
			}

			ImGui::TreePop();
		}

		ImGui::Separator();

		const char* lightingTypeNames[] = {"None", "Lambert", "Half Lambert", "Phong", "Blinn-Phong"};
		static int currentLightingType = static_cast<int>(LightingType::kHalfLambert);

		if (ImGui::Combo("Lighting Type", &currentLightingType, lightingTypeNames, IM_ARRAYSIZE(lightingTypeNames))) {
			sphere_->SetLightingType(static_cast<LightingType>(currentLightingType));
		}
	}
	*/
#endif
}

void TitleScene::DrawEditorHierarchyAndInspector() {
#ifdef USE_IMGUI
	// DevEditorのヒエラルキーとインスペクター登録
	DevEditor::GetInstance()->DrawHierarchy("タイトルシーン ヒエラルキー", [this]() {
		
		// Title Stage Scene Settings
		if (DevEditor::GetInstance()->HierarchyNode("タイトルステージ (グローバル)", this)) {
			DevEditor::GetInstance()->SetInspectorDrawer([this]() {
				DrawSceneInspector();
			});
		}
		// 1. Sphere
		if (sphere_) {
			if (DevEditor::GetInstance()->HierarchyNode("球体オブジェクト", sphere_.get())) {
				DevEditor::GetInstance()->SetInspectorDrawer([this]() {
					WorldTransform* wt = const_cast<WorldTransform*>(sphere_->GetWorldTransform());
					DevEditor::GetInstance()->DrawTransformEdit(wt);
					
					Vector4 color = sphere_->GetColor();
					float col[4] = {color.x, color.y, color.z, color.w};
					if (ImGui::ColorEdit4("色", col)) {
						sphere_->SetColor({col[0], col[1], col[2], col[3]});
					}
					
					float envCoeff = sphere_->GetEnvironmentCoefficient();
					if (ImGui::SliderFloat("反射強度", &envCoeff, 0.0f, 1.0f)) {
						sphere_->SetEnvironmentCoefficient(envCoeff);
					}
				});
			}
		}

		// 2. Primitive
		if (primitive_) {
			if (DevEditor::GetInstance()->HierarchyNode("プリミティブオブジェクト", primitive_.get())) {
				DevEditor::GetInstance()->SetInspectorDrawer([this]() {
					ImGui::Text("プリミティブ設定");
				});
			}
		}

		// 3. Sprite
		if (sprite_) {
			if (DevEditor::GetInstance()->HierarchyNode("スプライトオブジェクト", sprite_.get())) {
				DevEditor::GetInstance()->SetInspectorDrawer([this]() {
					ImGui::Text("スプライト設定");
				});
			}
		}

		// 4. Camera
		if (camera_) {
			if (DevEditor::GetInstance()->HierarchyNode("タイトルカメラ", camera_.get())) {
				DevEditor::GetInstance()->SetInspectorDrawer([this]() {
					ImGui::Checkbox("デバッグカメラを使用", &useDebugCamera_);
					ImGui::Separator();
					Vector3 pos = camera_->GetTranslate();
					if (ImGui::DragFloat3("座標", &pos.x, 0.1f)) {
						camera_->SetTranslate(pos);
						camera_->CalculateMatrix();
					}
					Vector3 rot = camera_->GetRotate();
					if (ImGui::DragFloat3("回転", &rot.x, 0.01f)) {
						camera_->SetRotate(rot);
						camera_->CalculateMatrix();
					}
				});
			}
		}
	});
#endif
}
