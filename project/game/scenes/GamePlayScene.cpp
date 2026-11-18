#include "GamePlayScene.h"

#include <MyEngine.h>
#include "LightManager.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

GamePlayScene::GamePlayScene() = default;
GamePlayScene::~GamePlayScene() = default;

void GamePlayScene::Initialize() {
	// カメラの初期化
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({ 0.3f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 4.0f, -10.0f });
	camera_->CalculateMatrix();
	camera_->CreateConstantBuffer();

	// デバッグカメラの初期化
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize();
	debugCamera_->SetRotate(camera_->GetRotate());
	debugCamera_->SetTranslate(camera_->GetTranslate());
	debugCamera_->CalculateMatrix();
	debugCamera_->CreateConstantBuffer();

	// 平行光源の設定
	LightManager* lightManager = LightManager::GetInstance();
	lightManager->SetDirectionalLightColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	lightManager->SetDirectionalLightDirection({ 0.0f, -1.0f, 0.5f });
	lightManager->SetDirectionalLightIntensity(1.0f);

	// AnimatedCube の生成・初期化
	animatedCube_ = std::make_unique<Object3d>();
	animatedCube_->Initialize("AnimatedCube", "gltf");
	animatedCube_->SetCamera(camera_.get());
	animatedCube_->SetLightingType(LightingType::kHalfLambert);

	// アニメーションの読み込み・セット
	cubeAnimation_ = LoadAnimationFile("resources/models/AnimatedCube", "AnimatedCube.gltf");
	animatedCube_->SetAnimation(cubeAnimation_);
}

void GamePlayScene::Update() {
#ifdef USE_IMGUI
	ImGui::Begin("AnimatedCube");
	ImGui::Checkbox("Use Debug Camera", &useDebugCamera_);
	if (ImGui::Checkbox("Playing", &isAnimationPlaying_)) {
		animatedCube_->SetAnimationPlaying(isAnimationPlaying_);
	}
	if (ImGui::Checkbox("Loop", &isAnimationLoop_)) {
		animatedCube_->SetAnimationLoop(isAnimationLoop_);
	}
	if (ImGui::Button("Reset Animation")) {
		animatedCube_->ResetAnimation();
	}
	ImGui::Text("Animation Time: %.2f / %.2f sec", animatedCube_->GetAnimationTime(), cubeAnimation_.duration);
	ImGui::End();
#endif

	if (useDebugCamera_) {
		debugCamera_->Update(camera_.get());
	} else {
		camera_->CalculateMatrix();
	}

	if (animatedCube_) {
		animatedCube_->Update();
	}
}

void GamePlayScene::Draw() {
	if (animatedCube_) {
		animatedCube_->Draw();
	}
}

void GamePlayScene::Finalize() {
}
