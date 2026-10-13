#include "GameManager.h"

#include "GameSceneFactory.h"
#include "SceneManager.h"
#include "CrtTransition.h"

GameManager::GameManager() = default;

GameManager::~GameManager() = default;

void GameManager::Initialize() {
	// 基底クラスの初期化
	GameFramework::Initialize();

	// シーン遷移演出の初期化
	CrtTransition::GetInstance()->Initialize();

	// シーンファクトリを生成してマネージャにセット
	sceneFactory_ = std::make_unique<GameSceneFactory>();

	SceneManager::GetInstance()->SetSceneFactory(sceneFactory_.get());

	// シーンマネージャに最初のシーンをセット
	SceneManager::GetInstance()->ChangeScene("TITLE");
}

void GameManager::Update() {

	// 基底クラスの更新
	GameFramework::Update();

	SceneManager::GetInstance()->Update();

	// 画面遷移演出マネージャーの更新
	CrtTransition::GetInstance()->Update();
}

void GameManager::Draw() { SceneManager::GetInstance()->Draw(); }

void GameManager::Finalize() {
	// シーン遷移演出を解放
	CrtTransition::Finalize();

	// シーンマネージャを解放
	SceneManager::GetInstance()->Finalize();

	// 基底クラスの終了処理
	GameFramework::Finalize();

	CoUninitialize();
}
