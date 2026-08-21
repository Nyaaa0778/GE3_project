#pragma once

#include "EnemyBase.h"

/// <summary>
/// ドローンの飛行パターン
/// </summary>
enum class DroneFlightPattern {
	kSineWave,   // サイン波ウェーブ（奥から手前へ前進しつつ上下/左右にうねる）
	kCircle,     // 円旋回（基準位置の周りを旋回しながら前進）
	kSlalom,     // スラローム（大きく左右に蛇行しながら前進）
	kFigureEight // 8の字旋回
};

/// <summary>
/// 編隊飛行小型ドローン
/// </summary>
class FormationDroneEnemy : public EnemyBase {
public:
	/// <summary>
	/// 編隊パラメータ設定
	/// </summary>
	struct FormationConfig {
		DroneFlightPattern pattern = DroneFlightPattern::kSineWave;
		float phaseOffset = 0.0f;       // 編隊内の位相差（時間オフセット）
		Vector3 localOffset = {0.0f, 0.0f, 0.0f}; // 編隊内の相対配置オフセット
		float speed = 1.0f;            // 進行速度
		float waveAmplitude = 4.0f;     // サイン波振幅 / 旋回半径
		float waveFrequency = 2.5f;     // 周期・周波数
		float lifeTime = 12.0f;         // 生存時間（画面外退避後の自動消滅）
		Vector3 moveDirection = {0.0f, 0.0f, -1.0f}; // 進行基準方向（手前など）
	};

public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="model">描画モデル</param>
	/// <param name="camera">カメラ</param>
	/// <param name="basePos">初期基準座標</param>
	/// <param name="config">編隊飛行パラメータ</param>
	void Initialize(Object3d* model, Camera* camera, const Vector3& basePos, const FormationConfig& config = {});

	/// <summary>
	/// 更新
	/// </summary>
	void Update() override;

	/// <summary>
	/// 衝突時の処理
	/// </summary>
	void OnCollision() override;

	/// <summary>
	/// スコア値取得
	/// </summary>
	int GetScore() const override { return 150; }

private:
	// 各飛行パターンの位置計算
	Vector3 CalculatePosition(float t);

	// 移動方向に応じた姿勢（回転）の更新
	void UpdateRotation(const Vector3& velocity);

private:
	FormationConfig config_;
	Vector3 basePos_ = {0.0f, 0.0f, 0.0f};
	float elapsedTime_ = 0.0f;

	// 当たり判定の大きさ (小型ドローン用)
	static constexpr Vector3 kCollisionSize = {0.8f, 0.8f, 0.8f};
};
