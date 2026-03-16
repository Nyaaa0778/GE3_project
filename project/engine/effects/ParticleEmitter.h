#pragma once

#include <Transform.h>
#include <string>

class ParticleEmitter {
public:
	//================================================================================
	// コンストラクタ / 更新 / 描画
	//================================================================================

	/// <summary>
	/// パーティクルエミッタを生成
	/// </summary>
	/// <param name="groupName">発生させるパーティクルグループ名</param>
	/// <param name="targetTransform">パーティクルの発生位置</param>
	/// <param name="frequency">パーティクルの発生間隔（秒）</param>
	/// <param name="count">1回の発生で生成するパーティクル数</param>
	/// <param name="active">エミッタの有効／無効フラグ</param>
	ParticleEmitter(const std::string& groupName, Transform* targetTransform,
		float frequency, uint32_t count, bool active = true);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// パーティクル生成
	/// </summary>
	void Emit();

private:
	//================================================================================
	// エミッタ設定 / 状態
	//================================================================================

	// グループの名前
	std::string groupName_;

	// トランスフォーム
	Transform* transform_ = nullptr;

	// 秒
	float frequency_ = 0.1f;
	// 経過時間
	float frequencyTime_ = 0.0f;

	uint32_t count_ = 1;
	bool isActive_ = true;
};
