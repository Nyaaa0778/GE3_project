#pragma once

#include <vector>
#include <memory>
#include "Vector3.h"
#include "Vector4.h"

class Camera;
class WireframeObject;

/// <summary>
/// サイバーパンク風ワイヤーフレーム背景都市
/// </summary>
class CityBackground {
public:
	CityBackground();
	~CityBackground();

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="camera">描画に使用するカメラ</param>
	void Initialize(Camera* camera);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// カメラの再設定
	/// </summary>
	void SetCamera(Camera* camera);

	/// <summary>
	/// 背景スクロール速度の設定 (0で静止)
	/// </summary>
	void SetScrollSpeed(float speed) { scrollSpeed_ = speed; }
	float GetScrollSpeed() const { return scrollSpeed_; }

	/// <summary>
	/// ネオン明滅アニメーションの有効/無効
	/// </summary>
	void SetPulseEnabled(bool enabled) { enablePulse_ = enabled; }

private:
	struct Building {
		std::unique_ptr<WireframeObject> wireObject;
		Vector3 basePosition;
		Vector3 scale;
		Vector4 baseColor;
		float pulsePhase = 0.0f;
		float pulseSpeed = 1.0f;
		bool isFlickering = false;
		float flickerTimer = 0.0f;
	};

	Camera* camera_ = nullptr;

	// 地面のサイバーネオングリッド (二重配置でシームレススクロール)
	std::unique_ptr<WireframeObject> groundGrid1_;
	std::unique_ptr<WireframeObject> groundGrid2_;
	float gridLength_ = 100.0f;
	float gridZ_ = 0.0f;

	// 奥の巨大ネオンサン (Synthwave / Cyberpunk風)
	std::unique_ptr<WireframeObject> neonSun_;

	// 摩天楼ビル群
	std::vector<Building> buildings_;

	float scrollSpeed_ = 0.0f; // スクロール速度 (0で固定静止)
	bool enablePulse_ = true;
	float time_ = 0.0f;

private:
	void GenerateCity();
};
