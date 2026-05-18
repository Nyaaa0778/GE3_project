#include "GameManager.h"

#include "GameSceneFactory.h"
#include "IScene.h"
#include "SceneManager.h"

GameManager::GameManager() = default;

GameManager::~GameManager() = default;

void GameManager::Initialize() {
	// 基底クラスの初期化
	GameFramework::Initialize();

	// シーンファクトリを生成してマネージャにセット
	sceneFactory_ = std::make_unique<GameSceneFactory>();

	SceneManager::GetInstance()->SetSceneFactory(sceneFactory_.get());

	// シーンマネージャに最初のシーンをセット
	SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
}

void GameManager::Update() {

	// 基底クラスの更新
	GameFramework::Update();

	SceneManager::GetInstance()->Update();
}

void GameManager::Draw() { SceneManager::GetInstance()->Draw(); }

void GameManager::Finalize() {
	// シーンマネージャを解放
	SceneManager::GetInstance()->Finalize();

	// 基底クラスの終了処理
	GameFramework::Finalize();

	CoUninitialize();
}
