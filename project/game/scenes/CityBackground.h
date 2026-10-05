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
	/// <param name="minZ">ビルの配置開始Z座標</param>
	/// <param name="maxZ">ビルの配置終了Z座標</param>
	/// <param name="numBuildingsPerSide">片側あたりのビル数</param>
	/// <param name="roadHalfWidth">中央道路の半幅</param>
	/// <param name="gridLength">地面グリッドの長さ</param>
	void Initialize(Camera* camera, float minZ = 5.0f, float maxZ = 120.0f, int numBuildingsPerSide = 25, float roadHalfWidth = 8.0f, float gridLength = 100.0f);

	/// <summary>
	/// コース（スプライン軌道）に沿ってゴール地点までビル群を生成
	/// </summary>
	/// <param name="camera">描画に使用するカメラ</param>
	/// <param name="pathPoints">コースのスプライン制御点</param>
	/// <param name="roadHalfWidth">コース中央道路の半幅</param>
	/// <param name="buildingInterval">建物の配置間隔</param>
	/// <param name="extraEndMargin">ゴール地点の先へ延長配置する余白距離</param>
	void InitializeAlongPath(Camera* camera, const std::vector<Vector3>& pathPoints, float roadHalfWidth = 10.0f, float buildingInterval = 8.0f, float extraEndMargin = 60.0f);

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

	/// <summary>
	/// 背景都市全体の基準高さ (Y座標) を設定
	/// </summary>
	void SetBaseY(float baseY);
	float GetBaseY() const { return baseY_; }

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

	float minZ_ = 5.0f;
	float maxZ_ = 120.0f;
	float roadHalfWidth_ = 8.0f;
	int numBuildingsPerSide_ = 25;
	float baseY_ = -6.0f; // 基準の高さ (カメラより下目に配置)

private:
	void GenerateCity();
};
