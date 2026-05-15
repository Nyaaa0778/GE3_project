#include "TitleScene.h"

#include <MyEngine.h>
#include "LightManager.h"
#include <numbers>

#include "Skybox.h"
#include "DebugCamera.h"
#include "Ring.h"

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

	primitive_ = std::make_unique<Ring>();
	primitive_->Initialize("gradationLine.png");
	primitive_->SetCamera(camera_.get());
	// Plane へのキャストが必要な初期設定があればここで行うか、Plane内で完結させる
	if (auto* ring = dynamic_cast<Ring*>(primitive_.get())) {
		ring->SetPosition({ 2.0f, 0.0f, 0.0f }); // 少しずらしておく
	}

	// spriteの初期化
	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize("monsterBall.png");

	// --------------------------------------------------
	// ② ラジカセ（AudioSource）の実体を作る
	// --------------------------------------------------
	bgm_ = AudioManager::LoadAudio("title.mp3");
	se_ = AudioManager::LoadAudio("bgmGamePlay.wav", SoundGroup::SE);

	ParticleManager::GetInstance()->CreateParticleGroup("RingParticle", "resources/sprites/gradationLine.png", ParticleManager::ParticleShape::kRing);

	// 2. エミッタの生成（グループ名、追従するTransformのポインタ、発生間隔、1回の数）
	// titleTransform_ はタイトルロゴや背景の座標を指す想定
	emitter_ = std::make_unique<ParticleEmitter>("RingParticle", &particleTransform_, 0.2f, 3);
}

void TitleScene::Update() {
	auto input = Input::GetInstance();

	if (useDebugCamera_) {
		// ★ debugCameraController_ に camera_ を「操作してくれ」と頼む
		debugCamera_->Update(camera_.get());
	} else {
		// 通常時のカメラ挙動（固定やパス移動など）
	camera_->CalculateMatrix();
	}

	/*if (emitter_) {
		emitter_->Update();
	}

	ParticleManager::GetInstance()->Update(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());*/

	// 【効果音のテスト】スペースキーを押したら「決定音」を鳴らす
	if (input->TriggerKey(DIK_SPACE)) {
		AudioManager::PlayAudio(se_); // 何も書かない、または false で1回だけ再生
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
	float speed = 0.1f; // 通常速度

	// 左スティック押し込み (L3) を判定
	if (input->PushButton(XINPUT_GAMEPAD_LEFT_THUMB)) {
		speed = 0.3f; // ダッシュ速度
		// 振動させる（左モーター：低周波でガタガタ、右モーター：高周波で細かく）
		input->SetShake(0.3f, 0.7f);
	} else {
		// 離していたら振動を止める
		input->SetShake(0.0f, 0.0f);
	}

	if (input->TriggerButton(XINPUT_GAMEPAD_RIGHT_THUMB)) {
		input->SetShake(1.0f, 1.0f, 3.0f); // 1秒間ドカン！と震えて勝手に止まる
	}
	// --- 3. スティック移動処理 ---
	/*Input::Stick lStick = input->GetLeftStick();
	Vector3 pos = sphere_->GetPosition();
	pos.x += lStick.x * speed;
	pos.y += lStick.y * speed;
	sphere_->SetPosition(pos);*/

	sphere_->Update();
	//terrain_->Update();

	primitive_->Update();

	UpdateImGui();

	sprite_->Update();
}

void TitleScene::Draw() {

	sphere_->Draw();
	//terrain_->Draw();

	primitive_->Draw();
	//skybox_->Draw();

	// パーティクルの描画（インスタンシング描画が実行される）
	//ParticleManager::GetInstance()->Draw();

	//sprite_->Draw();
}

void TitleScene::Finalize() {

}

void TitleScene::UpdateImGui() {
#ifdef USE_IMGUI

	// ウィンドウ全体のサイズと位置の初期設定（必要に応じて調整してください）
	ImGui::SetNextWindowSize(ImVec2(400.0f, 500.0f), ImGuiCond_Once);

	// 1つの大きな親ウィンドウを作成
	ImGui::Begin("Debug Menu", nullptr);

	// ==========================================
	// 1. Sprite Control
	// ==========================================
	if (ImGui::CollapsingHeader("Sprite Control")) {
		Vector2 pos = sprite_->GetPosition();
		// ※ImGuiは同じラベル名("Position"など)が複数あると誤動作するため、名前を固有にしています
		if (ImGui::DragFloat2("Sprite Position", &pos.x, 1.0f, 0.0f, 0.0f, "%.1f")) {
			sprite_->SetPosition(pos);
		}

		Vector2 scale = sprite_->GetScale();
		if (ImGui::DragFloat2("Sprite Scale", &scale.x, 0.1f, -10.0f, 10.0f, "%.1f")) {
			sprite_->SetScale(scale);
		}

		float rot = sprite_->GetRotate();
		if (ImGui::DragFloat("Sprite Rotation", &rot, 0.1f, -6.28f, 6.28f, "%.1f")) {
			sprite_->SetRotation(rot);
		}

		Vector4 color = sprite_->GetColor();
		float col[4] = {color.x, color.y, color.z, color.w};
		if (ImGui::ColorEdit4("Sprite Color", col)) {
			Vector4 newColor(col[0], col[1], col[2], col[3]);
			sprite_->SetColor(newColor);
		}
	}

	// ==========================================
	// 2. Camera Control
	// ==========================================
	if (ImGui::CollapsingHeader("Camera Control")) {
		Vector3 camPos = camera_->GetTranslate();
		if (ImGui::DragFloat3("Camera Position", &camPos.x, 0.1f)) {
			camera_->SetTranslate(camPos);
		}

		Vector3 camRot = camera_->GetRotate();
		if (ImGui::DragFloat3("Camera Rotation", &camRot.x, 0.01f)) {
			camera_->SetRotate(camRot);
		}

		if (ImGui::Button("Reset Camera")) {
			camera_->SetTranslate({0.0f, 0.0f, -10.0f});
			camera_->SetRotate({0.0f, 0.0f, 0.0f});
		}
	}

	// ==========================================
	// 3. Particle Debug
	// ==========================================
	if (ImGui::CollapsingHeader("Particle Debug")) {
		auto& groups = ParticleManager::GetInstance()->GetGroups();
		for (auto& [name, group] : groups) {
			ImGui::Checkbox((name + " Billboard").c_str(), &group.useBillboard);
		}
	}

	// ==========================================
	// 4. Primitive Settings
	// ==========================================
	if (ImGui::CollapsingHeader("Primitive Settings")) {
		// ⚠️注意: primitive_->DrawImGui() の実装内で ImGui::Begin() / ImGui::End() が
		// 呼ばれている場合、このウィンドウとは別に独立して表示されてしまいます。
		// 1つのウィンドウにまとめるには、DrawImGui() 側の Begin/End を削除してください。
		primitive_->DrawImGui("Primitive Settings");
	}

	// ==========================================
	// 5. Object Settings
	// ==========================================
	if (ImGui::CollapsingHeader("Object Settings")) {
		Vector3 rotate = sphere_->GetRotate();
		if (ImGui::DragFloat3("Sphere Rotate", &rotate.x, 0.01f, 0.1f, 100.0f)) {
			sphere_->SetRotation(rotate);
		}

		float envCoeff = sphere_->GetEnvironmentCoefficient();
		if (ImGui::SliderFloat("Reflection Power", &envCoeff, 0.0f, 1.0f)) {
			sphere_->SetEnvironmentCoefficient(envCoeff);
		}
	}

	// ==========================================
	// 6. Debug Console
	// ==========================================
	if (ImGui::CollapsingHeader("Debug Console")) {
		if (ImGui::Checkbox("Use Debug Camera", &useDebugCamera_)) {
			if (useDebugCamera_) {
				debugCamera_->SetRotate(camera_->GetRotate());
				debugCamera_->SetTranslate(camera_->GetTranslate());
				debugCamera_->CalculateMatrix();
			}
		}
	}

	// ==========================================
	// 7. Light Settings (元々コメントアウトされていた部分)
	// ==========================================
	/*
	if (ImGui::CollapsingHeader("Light Settings")) {
		LightManager* lightManager = LightManager::GetInstance();

		if (ImGui::TreeNode("Directional Light")) {
			Vector4 color = lightManager->GetDirectionalLightColor();
			Vector3 direction = lightManager->GetDirectionalLightDirection();
			float intensity = lightManager->GetDirectionalLightIntensity();

			ImGui::ColorEdit4("Dir Color", &color.x);
			ImGui::DragFloat3("Dir Direction", &direction.x, 0.01f, -1.0f, 1.0f);
			ImGui::DragFloat("Dir Intensity", &intensity, 0.01f, 0.0f, 10.0f);

			lightManager->SetDirectionalLightColor(color);
			lightManager->SetDirectionalLightDirection(direction);
			lightManager->SetDirectionalLightIntensity(intensity);

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Local Light")) {
			// ... 略(元のコードのまま配置できます) ...
			ImGui::TreePop();
		}
	}
	*/

	// 親ウィンドウを閉じる（全体でこれ1つだけ！）
	ImGui::End();

#endif
}
