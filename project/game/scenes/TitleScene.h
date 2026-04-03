#pragma once

#include <memory>

#include "IScene.h"

class AudioSource;
class Object3d;
class Sprite;

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
	std::unique_ptr<Object3d> obj_;

	// スプライト
	std::unique_ptr<Sprite> sprite_;

	// 音声
	std::unique_ptr<AudioSource> bgm_; // BGM用
	std::unique_ptr<AudioSource> se_;  // 決定音やカーソル音などのSE用

private:
	void UpdateImGui();
};
