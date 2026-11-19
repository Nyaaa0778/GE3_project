#include "TitleScene.h"

#include <MyEngine.h>

TitleScene::TitleScene() = default;
TitleScene::~TitleScene() = default;

void TitleScene::Initialize() {
}

void TitleScene::Update() {
}

void TitleScene::Draw() {
}

void TitleScene::Finalize() {
}

void TitleScene::UpdateImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Cyberpunk Title Settings");
	ImGui::End();
#endif
}
