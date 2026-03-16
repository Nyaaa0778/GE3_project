#include "GamePlayScene.h"

#include "Object3d.h"
#include "Object3dRenderer.h"

#ifdef USE_IMGUI
#include "../externals/imgui/imgui.h"
#include "../externals/imgui/imgui_impl_dx12.h"
#include "../externals/imgui/imgui_impl_win32.h"
#endif

GamePlayScene::GamePlayScene() = default;
GamePlayScene::~GamePlayScene() = default;

void GamePlayScene::Initialize() {

	// object3dの初期化
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize("plane");
}

void GamePlayScene::Update() {
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

	{
		int mode = static_cast<int>(obj_->GetBlendMode());
		if (ImGui::Combo("BlendMode", &mode,
			"None\0Normal\0Add\0Subtract\0Multiply\0Screen")) {
			obj_->SetBlendMode(static_cast<Object3dRenderer::BlendMode>(mode));
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

void GamePlayScene::Draw() { obj_->Draw(); }

void GamePlayScene::Finalize() {}
