#pragma once

#include <memory>

#include "IScene.h"
#include "Transform.h"

class Object3d;
class Sprite;
class ParticleEmitter;
class Camera;
class DebugCamera;
class Skybox;
class WireframeObject;
class CityBackground;
#include "Plane.h"

class TitleScene : public IScene {
public:
	TitleScene();
	~TitleScene();

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

private:
	// サイバーパンク背景都市
	std::unique_ptr<CityBackground> cityBackground_;

	// ワイヤーフレーム
	std::unique_ptr<WireframeObject> wireBox_;

	// モデル
	std::unique_ptr<Object3d> sphere_;
	std::unique_ptr<Primitive> primitive_;

	// スプライト
	std::unique_ptr<Sprite> sprite_;

	// 音声
	uint32_t bgm_ = 0;
	uint32_t se_ = 0;

	// パーティクル
	std::unique_ptr<ParticleEmitter> emitter_;
	Transform particleTransform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10.0f}};
	
	// カメラ
	std::unique_ptr<Camera> camera_;
	std::unique_ptr<DebugCamera> debugCamera_;
	bool useDebugCamera_ = true;

	std::unique_ptr<Skybox> skybox_;

private:
	void UpdateImGui();

	// ディゾルブのアニメーション制御用変数
	float dissolveThreshold_ = 0.0f;
	bool isDissolving_ = false;
	bool isFadingOut_ = false;
	float dissolveSpeed_ = 0.01f;

	// サイバーパンク・グリッチ演出設定
	bool enableGlitch_ = true;
	float glitchIntensity_ = 0.5f;
	float chromaticAberration_ = 0.008f;
	float scanlineIntensity_ = 0.18f;
};
