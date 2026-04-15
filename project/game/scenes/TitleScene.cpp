#include "TitleScene.h"

#include <MyEngine.h>
#include "LightManager.h"
#include <numbers>

TitleScene::TitleScene() = default;
TitleScene::~TitleScene() = default;

void TitleScene::Initialize() {
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({0.5f, 0.0f, 0.0f});
	camera_->SetTranslate({0.0f, 8.0f, -15.0f});
	camera_->CreateConstantBuffer();

	// object3dの初期化
	sphere_ = std::make_unique<Object3d>();
	sphere_->Initialize("sphere");
	sphere_->SetCamera(camera_.get());

	terrain_ = std::make_unique<Object3d>();
	terrain_->Initialize("terrain");
	terrain_->SetCamera(camera_.get());

	// spriteの初期化
	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize("monsterBall.png");

	// --------------------------------------------------
	// ② ラジカセ（AudioSource）の実体を作る
	// --------------------------------------------------
	bgm_ = AudioManager::LoadAudio("title.mp3");
	se_ = AudioManager::LoadAudio("bgmGamePlay.wav", SoundGroup::SE);

	ParticleManager::GetInstance()->CreateParticleGroup("CircleParticle", "resources/sprites/circle.png");

	// 2. エミッタの生成（グループ名、追従するTransformのポインタ、発生間隔、1回の数）
	// titleTransform_ はタイトルロゴや背景の座標を指す想定
	emitter_ = std::make_unique<ParticleEmitter>("CircleParticle", &particleTransform_, 0.2f, 3);
}

void TitleScene::Update() {
	auto input = Input::GetInstance();

	camera_->Update();

	if (emitter_) {
		emitter_->Update();
	}

	ParticleManager::GetInstance()->Update(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());

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
	Input::Stick lStick = input->GetLeftStick();
	Vector3 pos = sphere_->GetPosition();
	pos.x += lStick.x * speed;
	pos.y += lStick.y * speed;
	sphere_->SetPosition(pos);

	sphere_->Update();
	terrain_->Update();

	UpdateImGui();

	sprite_->Update();
}

void TitleScene::Draw() {
	// パーティクルの描画（インスタンシング描画が実行される）
	ParticleManager::GetInstance()->Draw();

	sphere_->Draw();
	terrain_->Draw();

	//sprite_->Draw();
}

void TitleScene::Finalize() {

}

void TitleScene::UpdateImGui() {
#ifdef USE_IMGUI

	// 【要件クリア】1. ウィンドウのサイズを固定する (例: 幅400, 高さ300)
	// ImGuiCond_Once
	// を指定すると、最初の1回だけサイズを設定し、以後はそのサイズを維持します。
	ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), ImGuiCond_Once);

	// もしユーザーのドラッグによるリサイズ自体を完全に禁止したい場合は、フラグを渡します。
	ImGui::Begin("Sprite Control", nullptr, ImGuiWindowFlags_NoResize);

	ImGui::SeparatorText("Sprite Object");

	// 【要件クリア】2. Spriteの操作 & 3. 小数点1桁表示 ("%.1f")

	// 位置 (2D想定で Vector2 にしていますが、エンジン仕様が Vector3 なら
	// DragFloat3 にしてください)
	{
		Vector2 pos = sprite_->GetPosition();
		// 引数: ラベル, 変数のアドレス, 変化量, 最小値, 最大値(0で制限なし),
		// フォーマット
		if (ImGui::DragFloat2("Position", &pos.x, 1.0f, 0.0f, 0.0f, "%.1f")) {
			sprite_->SetPosition(pos);
		}
	}

	// スケール
	{
		Vector2 scale = sprite_->GetScale();
		if (ImGui::DragFloat2("Scale", &scale.x, 0.1f, -10.0f, 10.0f, "%.1f")) {
			sprite_->SetScale(scale);
		}
	}

	// 回転 (2DのZ軸回転を想定し、単一の float で扱う場合)
	{
		float rot = sprite_->GetRotate();
		if (ImGui::DragFloat("Rotation", &rot, 0.1f, -6.28f, 6.28f, "%.1f")) {
			sprite_->SetRotation(rot);
		}
	}

	// 色・透明度 (アルファ値)
	{
		Vector4 color = sprite_->GetColor();
		float col[4] = {color.x, color.y, color.z, color.w};

		if (ImGui::ColorEdit4("Color", col)) {
			Vector4 newColor(col[0], col[1], col[2], col[3]);
			sprite_->SetColor(newColor);
		}
	}

	ImGui::End();

	// 1. カメラ操作用のウィンドウを作成
	ImGui::Begin("Camera Control");

	// 座標（Translation）の調整
	// ※Cameraクラスのメンバ関数名が GetTranslate/SetTranslate であると仮定しています。
	//   もしエラーが出る場合は GetPosition/SetPosition に読み替えてください。
	Vector3 camPos = camera_->GetTranslate();
	if (ImGui::DragFloat3("Position", &camPos.x, 0.1f)) {
		camera_->SetTranslate(camPos);
	}

	// 回転（Rotation）の調整
	Vector3 camRot = camera_->GetRotate();
	if (ImGui::DragFloat3("Rotation", &camRot.x, 0.01f)) {
		camera_->SetRotate(camRot);
	}

	// リセットボタン（あると便利です）
	if (ImGui::Button("Reset Camera")) {
		camera_->SetTranslate({0.0f, 0.0f, -10.0f}); // 初期位置へ
		camera_->SetRotate({0.0f, 0.0f, 0.0f});
	}

	ImGui::End();

	if (ImGui::Begin("Particle Debug")) {
		// 特定のグループ（例: "CircleParticle"）のビルボード設定をいじる
		// ※実際には全グループをループで回して表示するとより便利です
		auto& groups = ParticleManager::GetInstance()->GetGroups(); // GetGroups()を自作して参照を返す
		for (auto& [name, group] : groups) {
			ImGui::Checkbox((name + " Billboard").c_str(), &group.useBillboard);
		}
	}

	ImGui::End();

	ImGui::Begin("Object Settings"); // 新しいウィンドウを作る場合

	// 1. 現在のスケールを取得
	Vector3 scale = sphere_->GetScale();

	// 2. ImGuiでX, Y, Zの値を操作する
	// （0.01fは変化スピード。0.1f 〜 10.0f の間で制限をかけています）
	if (ImGui::DragFloat3("Sphere Scale", &scale.x, 0.01f, 0.1f, 10.0f)) {
		// 3. スライダーが動かされて値が変更されたら、Object3dにセットし直す
		sphere_->SetScale(scale);
	}

	ImGui::End();

	// ImGuiのウィンドウを作成
	ImGui::Begin("Light Settings");

	// LightManagerのインスタンスを取得
	LightManager* lightManager = LightManager::GetInstance();

	// --------------------------------------------------
	// ① Directional Light の設定
	// --------------------------------------------------
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

		ImGui::TreePop(); // TreeNodeを閉じる
	}

	if (ImGui::TreeNode("Local Light")) {
		// 現在の状態を取得
		LocalLightType currentType = lightManager->GetLocalLightType();
		Vector4 color = lightManager->GetLocalLightColor();
		Vector3 position = lightManager->GetLocalLightPosition();
		float intensity = lightManager->GetLocalLightIntensity();
		float distance = lightManager->GetLocalLightDistance();
		float decay = lightManager->GetLocalLightDecay();

		// 1. ライトの種類切り替え
		const char* localLightTypeNames[] = {"Point", "Spot"};
		int typeIndex = static_cast<int>(currentType);
		if (ImGui::Combo("Light Type", &typeIndex, localLightTypeNames, IM_ARRAYSIZE(localLightTypeNames))) {
			lightManager->SetLocalLightType(static_cast<LocalLightType>(typeIndex));
		}

		ImGui::Separator();

		// 2. 共通設定
		ImGui::ColorEdit4("Color", &color.x);
		ImGui::DragFloat3("Position", &position.x, 0.1f);
		ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100.0f);
		ImGui::DragFloat("Distance", &distance, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("Decay", &decay, 0.01f, 0.0f, 10.0f);

		// 反映
		lightManager->SetLocalLightColor(color);
		lightManager->SetLocalLightPosition(position);
		lightManager->SetLocalLightIntensity(intensity);
		lightManager->SetLocalLightDistance(distance);
		lightManager->SetLocalLightDecay(decay);

		// 3. スポットライト専用設定
		if (currentType == LocalLightType::kSpot) {
			ImGui::Text("Spotlight Settings");

			Vector3 direction = lightManager->GetLocalLightDirection();
			float cosAngle = lightManager->GetLocalLightCosAngle();
			float cosFalloff = lightManager->GetLocalLightCosFalloffStart();

			// コサインを角度(度数法)に戻して表示
			float angleDeg = std::acos(cosAngle) * 180.0f / std::numbers::pi_v<float>;
			float falloffDeg = std::acos(cosFalloff) * 180.0f / std::numbers::pi_v<float>;

			ImGui::DragFloat3("Direction", &direction.x, 0.01f, -1.0f, 1.0f);
			if (ImGui::DragFloat("Angle", &angleDeg, 0.1f, 0.0f, 90.0f)) {
				// 変更があったらコサインに変換してセット
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

	// ==========================================
	// ここから追加：ライティングの種類の変更
	// ==========================================
	ImGui::Separator(); // 区切り線

	// プルダウンに表示する名前の配列（LightingTypeの順番に合わせる）
	const char* lightingTypeNames[] = {"None", "Lambert", "Half Lambert", "Phong", "Blinn-Phong"};

	// 現在選択されている種類のインデックス（初期値はModelの初期化に合わせておく）
	static int currentLightingType = static_cast<int>(LightingType::kHalfLambert);

	// コンボボックスで変更があった場合、Object3d に反映させる
	if (ImGui::Combo("Lighting Type", &currentLightingType, lightingTypeNames, IM_ARRAYSIZE(lightingTypeNames))) {
		sphere_->SetLightingType(static_cast<LightingType>(currentLightingType));
	}
	// ==========================================

	ImGui::End();

#endif
}
