#include "TitleScene.h"

#include "ImGuiManager.h"
#include "Input.h"
#include "Object3d.h"
#include "SceneManager.h"
#include "Sprite.h"
#include "AudioManager.h"

TitleScene::TitleScene() = default;
TitleScene::~TitleScene() = default;

void TitleScene::Initialize() {
	// object3dの初期化
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize("plane");

	// spriteの初期化
	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize("monsterBall.png");

	// --------------------------------------------------
	// ② ラジカセ（AudioSource）の実体を作る
	// --------------------------------------------------
	bgm_ = AudioManager::LoadAudio("title.mp3");
	se_ = AudioManager::LoadAudio("bgmGamePlay.wav", SoundGroup::SE);

	// --------------------------------------------------
	// ③ BGMをセットして、ループ再生スタート！
	// --------------------------------------------------
	
	AudioManager::PlayAudio(bgm_, true); // trueを渡すとループ再生！
}

void TitleScene::Update() {
	auto input = Input::GetInstance();

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
	Vector3 pos = obj_->GetPosition();
	pos.x += lStick.x * speed;
	pos.y += lStick.y * speed;
	obj_->SetPosition(pos);

	obj_->Update();

	UpdateImGui();

	sprite_->Update();
}

void TitleScene::Draw() {
	obj_->Draw();

	sprite_->Draw();
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

	static float volume = 1.0f;
	static float pitch = 1.0f;
	static float muffle = 0.0f;

	ImGui::Begin("BGM Controller");

	static float globalBgmVolume = 1.0f;
	if (ImGui::SliderFloat("Global BGM Volume", &globalBgmVolume, 0.0f, 1.0f)) {
		AudioManager::SetBGMVolume(globalBgmVolume);
	}

	static float globalSeVolume = 1.0f;
	if (ImGui::SliderFloat("Global SE Volume", &globalSeVolume, 0.0f, 1.0f)) {
		AudioManager::SetSEVolume(globalSeVolume);
	}

	ImGui::End();

	// 2. ImGuiのウィンドウを作成
	ImGui::Begin("BGM Controller");

	if (AudioManager::IsPlaying(bgm_)) {
		// 再生中の場合は緑色のテキストで表示
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: Playing");
	} else {
		// 停止中の場合は赤色のテキストで表示
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Status: Stopped");
	}

	ImGui::Separator();

	static float pan = 0.0f; // 追加

	// ---------------------------------------------------
	// パラメータ調整（スライダー）
	// ---------------------------------------------------
	if (ImGui::SliderFloat("Pan", &pan, -1.0f, 1.0f)) {
		AudioManager::SetPan(bgm_, pan);
	}
	// ダブルクリック等で中央に戻せるようにリセットボタンを置くと便利です
	ImGui::SameLine();
	if (ImGui::Button("Reset Pan")) {
		pan = 0.0f;
		AudioManager::SetPan(bgm_, pan);
	}

	ImGui::Separator();

	// ---------------------------------------------------
	// パラメータ調整（スライダー）
	// ---------------------------------------------------
	// ImGui::SliderFloat は、値が変更されたフレームでのみ true を返します。
	if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f)) {
		AudioManager::SetMasterVolume(volume);
	}

	// ピッチの範囲はゲームの要件に合わせて調整してください（例：0.1〜2.0）
	if (ImGui::SliderFloat("Pitch", &pitch, 0.1f, 2.0f)) {
		AudioManager::SetPitch(bgm_, pitch);
	}

	if (ImGui::SliderFloat("Muffle", &muffle, 0.0f, 1.0f)) {
		AudioManager::SetMuffle(bgm_, muffle);
	}

	ImGui::Separator();

	// ---------------------------------------------------
	// 再生・停止コントロール（ボタン）
	// ---------------------------------------------------
	// セッターの確認には音を鳴らす必要があるので、ボタンも作っておくと便利です
	if (ImGui::Button("Play")) {
		AudioManager::PlayAudio(bgm_); // ループさせたい場合は PlayAudio(true)
	}
	ImGui::SameLine(); // 次のUIを右に並べる
	if (ImGui::Button("Pause")) {
		AudioManager::PauseAudio(bgm_);
	}
	ImGui::SameLine();
	if (ImGui::Button("Resume")) {
		AudioManager::ResumeAudio(bgm_);
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop")) {
		AudioManager::StopAudio(bgm_);
	}

	ImGui::End();

	ImGui::Begin("SE Controller");

	// ---------------------------------------------------
	// 再生・停止コントロール（ボタン）
	// ---------------------------------------------------
	// セッターの確認には音を鳴らす必要があるので、ボタンも作っておくと便利です
	if (ImGui::Button("Play")) {
		AudioManager::PlayAudio(se_); // ループさせたい場合は PlayAudio(true)
	}
	ImGui::SameLine(); // 次のUIを右に並べる
	if (ImGui::Button("Pause")) {
		AudioManager::PauseAudio(se_);
	}
	ImGui::SameLine();
	if (ImGui::Button("Resume")) {
		AudioManager::ResumeAudio(se_);
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop")) {
		AudioManager::StopAudio(se_);
	}
	ImGui::SameLine();
	if (ImGui::Button("One Shot")) {
		AudioManager::PlayOneShot(se_);
	}

	ImGui::End();

#endif
}
