#pragma once

#include <Vector3.h>

class Object3d;

class PlayerBullet {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="model">自機の弾のモデル</param>
	/// <param name="pos">初期位置</param>
	/// <param name="direction">飛んでいく方向（正規化済みのベクトル）</param>
	void Initialize(Object3d* model, const Vector3& pos, const Vector3& direction);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

private:

	// モデル
	Object3d* model_ = nullptr;

	// 位置
	Vector3 pos_ = {0.0f, 0.0f, 0.0f};
	// 速さ
	static inline const float kBaseSpeed = 0.5f;
	// 速度
	Vector3 velocity_ = {0.0f, 0.0f, 0.0f};

	// 当たり判定の大きさ
	static inline const Vector3 collisionSize_ = {1.0f, 1.0f, 1.0f};

private:
	/// <summary>
	/// 移動処理
	/// </summary>
	void UpdateMove();
};

