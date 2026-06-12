#include "GamePlayScene.h"

#include <MyEngine.h>

#include "LevelLoader.h"
#include "MathUtility.h"
#include "LightManager.h"

GamePlayScene::GamePlayScene() = default;
GamePlayScene::~GamePlayScene() = default;

void GamePlayScene::Initialize() {
	// カメラのインスタンス生成
	camera_ = std::make_unique<Camera>();

	// レベルデータのロード
	LevelLoader loader;
	std::unique_ptr<LevelData> levelData(loader.Load("TL1Sample"));

	// ロードしたカメラパラメータ（位置・回転）があれば設定、なければデフォルト値
	if (!levelData->cameras.empty()) {
		camera_->SetTranslate(levelData->cameras[0].translation);
		camera_->SetRotate(levelData->cameras[0].rotation);
	} else {
		camera_->SetRotate({0.3f, 0.0f, 0.0f});
		camera_->SetTranslate({0.0f, 6.0f, -20.0f});
	}
	camera_->CalculateMatrix();
	camera_->CreateConstantBuffer();

	// ロードしたライトパラメータ（位置・回転）があれば設定
	if (!levelData->lights.empty()) {
		const auto& lightData = levelData->lights[0];

		// 点光源（Local Light）の設定
		LightManager::GetInstance()->SetLocalLightPosition(lightData.translation);
		LightManager::GetInstance()->SetLocalLightIntensity(1.0f);
		LightManager::GetInstance()->SetLocalLightDistance(50.0f); // 届く範囲を拡張

		// 平行光源（Directional Light）は使用しないため無効化（強度 0.0f）
		LightManager::GetInstance()->SetDirectionalLightIntensity(0.0f);
	}

	// object3dの初期化
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize("sphere");
	obj_->SetCamera(camera_.get());

	// 読み込んだレベルデータのオブジェクトを生成・初期化
	for (const auto& objectData : levelData->objects) {
		if (objectData.filename.empty()) {
			continue;
		}
		auto newObj = std::make_unique<Object3d>();
		newObj->Initialize(objectData.filename);
		newObj->SetPosition(objectData.translation);
		newObj->SetRotation(objectData.rotation);
		newObj->SetScale(objectData.scaling);
		newObj->SetCamera(camera_.get());
		objects_.push_back(std::move(newObj));
	}

	for (const auto& spawner : levelData->spawners) {
		// ① プレイヤーの場合
		// エディタ側で "Player" や "PlayerSpawn" という名前をつけていると想定
		if (spawner.entityType.find("Player") != std::string::npos) {
			obj_->SetPosition(spawner.translation);
			obj_->SetRotation(spawner.rotation);
		}
		// ② 敵の場合
		else if (spawner.entityType.find("Enemy") != std::string::npos) {
			// 今回は一旦 Object3d として生成
			auto enemy = std::make_unique<Object3d>();
			enemy->Initialize("sphere"); // ※敵のモデル名に変更してください
			enemy->SetPosition(spawner.translation);
			enemy->SetRotation(spawner.rotation);
			enemy->SetCamera(camera_.get());

			// 一旦、背景と同じ objects_ に追加して描画されるようにする
			// （本格的に処理を分けるなら GamePlayScene.h に enemies_ 等の配列を作るのがおすすめ）
			objects_.push_back(std::move(enemy));
		}
		// ③ アイテムの場合
		else if (spawner.entityType.find("Item") != std::string::npos) {
			auto item = std::make_unique<Object3d>();
			item->Initialize("cube"); // ※アイテムのモデル名に変更してください
			item->SetPosition(spawner.translation);
			item->SetRotation(spawner.rotation);
			item->SetCamera(camera_.get());

			objects_.push_back(std::move(item));
		}
	}
}

void GamePlayScene::Update() {

	static float effectTime = 0.0f;
	effectTime += 0.001f; // アニメーションのスピード（数値を変えると速さが変わる）

	// 1. 横方向にUVスクロール (X座標を時間で動かす)
	primitive_->SetUVTranslation({effectTime, 0.0f});

	// 2. 色をアニメーションさせる (サイン波を使ってフワフワ色を変える)
	// 例: 青を強め(1.0)にしつつ、赤と緑を 0.0 ～ 1.0 の間で揺らす
	float r = std::sin(effectTime * 2.0f) * 0.5f + 0.5f;
	float g = std::cos(effectTime * 3.0f) * 0.5f + 0.5f;
	float b = 1.0f;
	primitive_->SetColor({r, g, b, 1.0f});

	primitive_->Update();

#ifdef USE_IMGUI

	ImGui::Begin("Window");

	//// ─────────────────────
	//// ライト
	//// ─────────────────────

	// ImGui::SeparatorText("Directional Light");

	// Vector4 color = object3d->GetLightColor();
	// if (ImGui::ColorEdit3("Light Color", &color.x)) {
	//   object3d->SetLightColor({color.x, color.y, color.z, 1.0f});
	// }

	// Vector3 dir = object3d->GetLightDirection();
	// if (ImGui::SliderFloat3("Direction", &dir.x, -1.0f, 1.0f)) {
	//   object3d->SetLightDirection(dir); // ← そのまま渡す
	// }

	// float intensity = object3d->GetLightIntensity();
	// if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 5.0f)) {
	//   object3d->SetLightIntensity(intensity);
	// }

	// ─────────────────────
	// Obj
	// ─────────────────────

	ImGui::SeparatorText("Object");

	{
		Vector3 pos = obj_->GetPosition();
		if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
			obj_->SetPosition(pos);
		}
	}

	{
		Vector3 scale = obj_->GetScale();
		if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, -10.0f, 10.0f)) {
			obj_->SetScale(scale);
		}
	}

	{
		Vector3 rot = obj_->GetRotate(); // ← 正しい！
		if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f, -6.28f, 6.28f)) {
			obj_->SetRotation(rot);
		}
	}

	{
		Vector4 color = obj_->GetColor();
		float col[4] = {color.x, color.y, color.z, color.w};

		// ImGui カラーピッカー
		if (ImGui::ColorEdit4("Color", col)) {

			// float[4] → Vector4 に戻す
			Vector4 newColor(col[0], col[1], col[2], col[3]);

			// Object3d 経由で Model に反映
			obj_->SetColor(newColor);
		}
	}

	// ─────────────────────
	// カメラ
	// ─────────────────────

	ImGui::SeparatorText("Camera");

	// 位置
	{
		Vector3 pos = camera_->GetTranslate();
		if (ImGui::DragFloat3("Camera Position", &pos.x, 0.1f)) {
			camera_->SetTranslate(pos);
		}
	}

	// 回転（ラジアン or 度はお好みで）
	{
		Vector3 rot = camera_->GetRotate();
		if (ImGui::DragFloat3("Camera Rotation", &rot.x, 0.01f)) {
			camera_->SetRotate(rot);
		}
	}

	ImGui::End();

#endif

	if (camera_) {
		camera_->CalculateMatrix();
	}

	obj_->Update();

	for (auto& obj : objects_) {
		obj->Update();
	}
}

void GamePlayScene::Draw() {
	obj_->Draw();

	for (auto& obj : objects_) {
		obj->Draw();
	}
}

void GamePlayScene::Finalize() {}
