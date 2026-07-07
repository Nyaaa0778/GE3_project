#pragma once

#include <Vector3.h>

class Shake {
public:
	/// <summary>
	/// シェイクを開始する
	/// </summary>
	/// <param name="duration">継続時間（秒）</param>
	/// <param name="intensity">最大の揺れの強さ</param>
	void Start(float duration, float intensity);

	/// <summary>
	/// 毎フレームの更新処理
	/// </summary>
	/// <param name="deltaTime">フレームの経過時間（秒）</param>
	void Update(float deltaTime);

	/// <summary>
	/// シェイクがアクティブかどうかを取得
	/// </summary>
	bool IsActive() const { return isActive_; }

	/// <summary>
	/// 現在の揺れのオフセットを取得
	/// </summary>
	const Vector3& GetOffset() const { return offset_; }

private:
	bool isActive_ = false;
	float duration_ = 0.0f;
	float timer_ = 0.0f;
	float intensity_ = 0.0f;
	Vector3 offset_ = {0.0f, 0.0f, 0.0f};
};
