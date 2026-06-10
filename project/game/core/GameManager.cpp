#include "GameManager.h"

#include "GameSceneFactory.h"
#include "SceneManager.h"

#ifdef USE_IMGUI
#include "DebugManager.h"
#endif

#include <objbase.h>

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

#ifdef USE_IMGUI
	// DebugManagerが一時停止中はゲームロジックを止める
	if (DebugManager::GetInstance()->IsPaused()) {
		return;
	}
#endif

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
