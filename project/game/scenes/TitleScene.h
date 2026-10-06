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
class TitleLogo;
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

	// サイバーパンクタイトルロゴ「CYBERAIL」
	std::unique_ptr<TitleLogo> titleLogo_;

	// スプライト
	std::unique_ptr<Sprite> backgroundSprite_;
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
	bool useDebugCamera_ = false;

	// 俯瞰オービットカメラ設定
	bool enableOrbitCamera_ = true;
	Vector3 orbitCenter_ = {0.0f, 4.0f, 50.0f}; // 周回中心点（サイバーシティの交差点/ストリート）
	float orbitRadius_ = 105.0f;                  // 周回半径
	float orbitHeight_ = 80.0f;                  // 中心からの高さオフセット (俯瞰)
	float orbitSpeed_ = 0.08f;                   // 周回速度 (ラジアン/秒)
	float orbitAngle_ = 0.0f;                    // 現在の周回角度

	std::unique_ptr<Skybox> skybox_;

private:
	void UpdateOrbitCamera();
	void UpdateImGui();

	// ディゾルブのアニメーション制御用変数
	float dissolveThreshold_ = 0.0f;
	bool isDissolving_ = false;
	bool isFadingOut_ = false;
	float dissolveSpeed_ = 0.01f;

	// サイバーパンク・グリッチ演出設定 (普段は平穏、数秒に1度だけ一瞬走る自然な間欠演出)
	bool enableGlitch_ = true;
	float glitchIntensity_ = 0.60f;      // 発生時の一瞬の乱れ強度
	float chromaticAberration_ = 0.008f; // 発生時の色ズレ強度
	float scanlineIntensity_ = 0.12f;    // 常時薄く乗る走査線 (0で無効)
	float glitchBlockCount_ = 35.0f;     // 画面縦の分割数
	float glitchShiftScale_ = 1.0f;      // 横ズレの振れ幅倍率

	// 間欠的グリッチのバースト制御タイマー
	float glitchTimer_ = 0.0f;
	float glitchNextInterval_ = 3.2f;    // 次の発生までの静寂時間 (2.5秒〜5.0秒)
	float glitchBurstDuration_ = 0.08f;  // 一瞬の持続時間 (0.05秒〜0.10秒: 数フレーム程度)
	float glitchBurstTimer_ = 0.0f;
	bool isGlitching_ = false;
	int glitchSubBurstCount_ = 0;        // たまに「パパッ」と二連撃を起こす用
};
