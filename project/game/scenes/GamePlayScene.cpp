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
	simpleSkin_ = std::make_unique<Object3d>();
	simpleSkin_->Initialize("human/walk", "gltf");
	simpleSkin_->SetCamera(camera_.get());
	simpleSkin_->SetLightingType(LightingType::kHalfLambert);

	// アニメーションの読み込み・セット
	simpleSkinAnimation_ = LoadAnimationFile("resources/models/human", "walk.gltf");
	simpleSkin_->SetAnimation(simpleSkinAnimation_);
	simpleSkin_->SetRotation({0.0f, 3.14159265f, 0.0f});

	// スケルトンの初期化
	if (const Node* rootNode = simpleSkin_->GetRootNode()) {
		simpleSkinSkelton_.Initialize(*rootNode);
	}
}

void GamePlayScene::Update() {
#ifdef USE_IMGUI
	ImGui::Begin("simpleSkin");
	ImGui::Checkbox("Use Debug Camera", &useDebugCamera_);
	ImGui::Checkbox("Draw Skelton", &drawSkelton_);
	if (ImGui::Checkbox("Playing", &isAnimationPlaying_)) {
		simpleSkin_->SetAnimationPlaying(isAnimationPlaying_);
	}
	if (ImGui::Checkbox("Loop", &isAnimationLoop_)) {
		simpleSkin_->SetAnimationLoop(isAnimationLoop_);
	}
	if (ImGui::Button("Reset Animation")) {
		simpleSkin_->ResetAnimation();
	}
	ImGui::Text("Animation Time: %.2f / %.2f sec", simpleSkin_->GetAnimationTime(), simpleSkinAnimation_.duration);
	ImGui::End();
#endif

	if (useDebugCamera_) {
		debugCamera_->Update(camera_.get());
	} else {
		camera_->CalculateMatrix();
	}

	if (simpleSkin_) {
		simpleSkin_->Update(simpleSkinSkelton_.GetSkeltonData());
	}
}

void GamePlayScene::Draw() {
	if (simpleSkin_) {
		simpleSkin_->Draw();
		if (drawSkelton_) {
			simpleSkinSkelton_.Draw(camera_.get(), simpleSkin_->GetWorldTransform()->matWorld);
		}
	}
}

void GamePlayScene::Finalize() {
}
