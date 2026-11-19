#include "GamePlayScene.h"

#include <MyEngine.h>

GamePlayScene::GamePlayScene() = default;
GamePlayScene::~GamePlayScene() = default;

void GamePlayScene::Initialize() {
}

void GamePlayScene::Update() {
}

void GamePlayScene::Draw() {
}


void GamePlayScene::Finalize() {
}


void GamePlayScene::UpdateImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("デバッグウィンドウ");

	ImGui::End();
#endif
}