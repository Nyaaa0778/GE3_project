#pragma once
#include <memory>
#include <vector>
#include <string>
#include <Vector3.h>
#include <Vector4.h>
#include "WorldTransform.h"

class Plane;
class Camera;

class Shockwave {
private:
	struct Ring {
		std::unique_ptr<Plane> plane;
		WorldTransform worldTransform = {};
		float delay = 0.0f;     // 開始遅延（秒）
		float elapsed = 0.0f;   // 経過時間（秒）
		float alpha = 0.0f;
		float maxScale = 1.0f;
		bool isFinished = false;
	};

public:
	Shockwave();
	~Shockwave();

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="camera">カメラへのポインタ</param>
	/// <param name="position">発生させる3D位置座標</param>
	void Initialize(Camera* camera, const Vector3& position);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// アニメーション終了フラグの取得
	/// </summary>
	bool IsFinished() const;

private:
	Camera* camera_ = nullptr;
	Vector3 position_ = {};
	std::vector<Ring> rings_;

	// 衝撃波エフェクトの各種パラメータ設定定数
	static constexpr size_t kRingCount = 3;             // 生成するリングの総数
	static constexpr float kRingDuration = 0.6f;        // 各リングの寿命（秒）
	static constexpr float kBaseMaxScale = 3.0f;        // 基準となる最大スケール（1番目のリング）
	static constexpr float kScaleDecreaseRatio = 0.8f;  // 後続リングのスケール減衰比率（0.8倍ずつ小さくする）
	static constexpr float kDelayInterval = 0.15f;      // リング同士の発生時間差（秒）
	
	// 「ほわほわん」フェードイン・フェードアウト比率
	static constexpr float kFadeInDurationRatio = 0.2f; // 寿命の最初の何割でフェードインするか（20%）
	static constexpr float kFadeOutDurationRatio = 1.0f - kFadeInDurationRatio; // 残りのフェードアウト割合（80%）
	
	// アニメーションイージング
	static constexpr float kEaseOutPower = 3.0f;        // イージング（Ease-out Cubic）の次数
	
	// 更新用フレームレート換算時間（60FPS基準）
	static constexpr float kFrameTime = 1.0f / 60.0f;

	// アセット名定数
	static inline const std::string kTextureName = "shockwave.png";

	// 浮動小数点数リテラルの定数化
	static constexpr float kZero = 0.0f;
	static constexpr float kOne = 1.0f;

	// カラーチャネル定数（白を基準に乗算する）
	static constexpr float kColorRed = 1.0f;
	static constexpr float kColorGreen = 1.0f;
	static constexpr float kColorBlue = 1.0f;

	// 行列およびトランスフォーム計算用定数
	static constexpr float kScaleZDefault = 1.0f;       // 2DのPlaneメッシュのため、Zスケールは1.0固定
	static constexpr float kMatrixTranslationW = 1.0f;  // 同次座標系におけるW値の初期設定値
};
