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
	/// <param name="fogNear">奥のフェード開始距離</param>
	/// <param name="fogFar">奥のフェード完全消滅距離</param>
	void Initialize(Camera* camera, float minZ = 4.0f, float maxZ = 320.0f, int numBuildingsPerSide = 65, float roadHalfWidth = 8.0f, float gridLength = 200.0f, float fogNear = 80.0f, float fogFar = 300.0f);

	/// <summary>
	/// コース（スプライン軌道）に沿ってゴール地点までビル群を生成
	/// </summary>
	/// <param name="camera">描画に使用するカメラ</param>
	/// <param name="pathPoints">コースのスプライン制御点</param>
	/// <param name="roadHalfWidth">コース中央道路の半幅</param>
	/// <param name="buildingInterval">建物の配置間隔</param>
	/// <param name="extraEndMargin">ゴール地点の先へ延長配置する余白距離</param>
	/// <param name="fogNear">奥のフェード開始距離</param>
	/// <param name="fogFar">奥のフェード完全消滅距離</param>
	void InitializeAlongPath(Camera* camera, const std::vector<Vector3>& pathPoints, float roadHalfWidth = 10.0f, float buildingInterval = 8.0f, float extraEndMargin = 150.0f, float fogNear = 80.0f, float fogFar = 300.0f);

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

	/// <summary>
	/// 距離フェード (フォグ) の設定
	/// </summary>
	void SetFog(float fogNear, float fogFar);
	float GetFogNear() const { return fogNear_; }
	float GetFogFar() const { return fogFar_; }

	/// <summary>
	/// 建物の段階的形成アニメーション (枝分かれ生成) の有効/無効
	/// </summary>
	void SetBuildAnimationEnabled(bool enabled);
	bool IsBuildAnimationEnabled() const { return buildAnimationEnabled_; }

	/// <summary>
	/// 形成アニメーションを最初からリプレイ
	/// </summary>
	void ResetBuildAnimation();

private:
	struct EdgeGrowth {
		int fromVertex = 0;
		int toVertex = 0;
		float startTime = 0.0f;
		float duration = 0.35f;
	};

	struct BuildingGrowth {
		float startDelay = 0.0f;
		std::vector<EdgeGrowth> edges;
		bool isComplete = false;
	};

	struct Building {
		std::unique_ptr<WireframeObject> wireObject;
		Vector3 basePosition;
		Vector3 scale;
		Vector4 baseColor;
		float pulsePhase = 0.0f;
		float pulseSpeed = 1.0f;
		bool isFlickering = false;
		float flickerTimer = 0.0f;

		BuildingGrowth growth;
	};

	Camera* camera_ = nullptr;

	// 地面のサイバーネオングリッド (二重配置でシームレススクロール)
	std::unique_ptr<WireframeObject> groundGrid1_;
	std::unique_ptr<WireframeObject> groundGrid2_;
	float gridLength_ = 200.0f;
	float gridZ_ = 0.0f;

	// 奥の巨大ネオンサン (Synthwave / Cyberpunk風)
	std::unique_ptr<WireframeObject> neonSun_;

	// 摩天楼ビル群
	std::vector<Building> buildings_;

	float scrollSpeed_ = 0.0f; // スクロール速度 (0で固定静止)
	bool enablePulse_ = true;
	float time_ = 0.0f;

	float minZ_ = 4.0f;
	float maxZ_ = 320.0f;
	float roadHalfWidth_ = 8.0f;
	int numBuildingsPerSide_ = 65;
	float baseY_ = -6.0f; // 基準の高さ (カメラより下目に配置)

	// 距離フォグ
	float fogNear_ = 80.0f;
	float fogFar_ = 300.0f;

	// 形成アニメーション制御
	bool buildAnimationEnabled_ = false;
	float buildAnimTime_ = 0.0f;

private:
	void GenerateCity();
	void SetupBuildAnimation();
	void UpdateBuildAnimation();
};
