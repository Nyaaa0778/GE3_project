#include "GamePlayScene.h"

#include <MyEngine.h>

#include "Cylinder.h"

using namespace std;

GamePlayScene::GamePlayScene() = default;
GamePlayScene::~GamePlayScene() = default;

void GamePlayScene::Initialize() {

	// object3dの初期化
	obj_ = make_unique<Object3d>();
	obj_->Initialize("plane");

	primitive_ = make_unique<Cylinder>();
	primitive_->Initialize("gradationLine.png");

	//primitive_->SetCamera(camera_);

	// 4. 各種パラメータの設定
	// 前回の修正により、Primitive型のポインタから直接これらのSetterが呼べます！
	primitive_->SetPosition({0.0f, 0.5f, 5.0f});
	primitive_->SetScale({1.0f, 1.0f, 1.0f}); // 縦に長い円柱など
	primitive_->SetRotation({0.0f, 0.0f, 0.0f});

	// 色やブレンドモード、テクスチャの変更も可能
	primitive_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	primitive_->SetBlendMode(PrimitiveRenderer::BlendMode::kAdd);
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

	//// ─────────────────────
	//// カメラ
	//// ─────────────────────

	// ImGui::SeparatorText("Camera");

	//// 位置
	//{
	//  Vector3 pos = camera_->GetTranslate();
	//  if (ImGui::DragFloat3("Camera Position", &pos.x, 0.1f)) {
	//    camera_->SetTranslate(pos);
	//  }
	//}

	//// 回転（ラジアン or 度はお好みで）
	//{
	//  Vector3 rot = camera_->GetRotate();
	//  if (ImGui::DragFloat3(" Camera Rotation", &rot.x, 0.01f)) {
	//    camera_->SetRotate(rot);
	//  }
	//}

	ImGui::End();

#endif

	obj_->Update();
}

void GamePlayScene::Draw() {
	obj_->Draw();

	primitive_->Draw();
}

void GamePlayScene::Finalize() {}
