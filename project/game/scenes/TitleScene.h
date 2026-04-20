#pragma once

#include <memory>

#include "IScene.h"
#include "Transform.h"

class Object3d;
class Sprite;
class ParticleEmitter;
class Camera;
class Skybox;

class TitleScene : public IScene {
public:
	TitleScene();
	~TitleScene();

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

private:
	// モデル
	/*std::unique_ptr<Object3d> sphere_;
	std::unique_ptr<Object3d> terrain_;*/
	std::unique_ptr<Object3d> plane_;

	// スプライト
	std::unique_ptr<Sprite> sprite_;

	// 音声
	uint32_t bgm_ = 0;
	uint32_t se_ = 0;

	// パーティクル
	std::unique_ptr<ParticleEmitter> emitter_;
	Transform particleTransform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10.0f}};
	std::unique_ptr<Camera> camera_;

	std::unique_ptr<Skybox> skybox_;

private:
	void UpdateImGui();
};
