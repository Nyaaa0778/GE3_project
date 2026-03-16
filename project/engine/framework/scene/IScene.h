#pragma once

class SceneManager;

class IScene {
public:
	virtual ~IScene() = default;

	virtual void Initialize() = 0;

	virtual void Update() = 0;

	virtual void Draw() = 0;

	virtual void Finalize() = 0;

public:
	virtual void SetSceneManager(SceneManager* sceneManager) {
		sceneManager_ = sceneManager;
	}

protected:
	// シーンマネージャ
	SceneManager* sceneManager_ = nullptr;
};
