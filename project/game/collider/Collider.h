#pragma once

#include <Vector3.h>

// 衝突形状タイプ
enum class ColliderShape {
	kSphere, // 球
	kAABB,   // AABB（軸平行境界ボックス）
};

// 球形状データ
struct Sphere {
	float radius = 1.0f;
};

// AABB形状データ
struct AABB {
	Vector3 size = { 1.0f, 1.0f, 1.0f };
};

class Collider {
public:
	virtual ~Collider() = default;

	// 衝突時のコールバック
	virtual void OnCollision() = 0;
	// ワールド座標を取得
	virtual Vector3 GetWorldPosition() = 0;
	// 前フレームのワールド座標を取得
	virtual Vector3 GetPrevWorldPosition() { return GetWorldPosition(); }

public:
	// 形状タイプのゲッター・セッター
	ColliderShape GetShape() const { return shape_; }
	void SetShape(ColliderShape shape) { shape_ = shape; }

	// 球データのゲッター・セッター
	const Sphere& GetSphere() const { return sphere_; }
	void SetSphere(const Sphere& sphere) { sphere_ = sphere; }

	// AABBデータのゲッター・セッター
	const AABB& GetAABB() const { return aabb_; }
	void SetAABB(const AABB& aabb) { aabb_ = aabb; }

	// AABBの最小・最大座標を取得（GetWorldPosition()を中心とする）
	Vector3 GetAABBMin();
	Vector3 GetAABBMax();
private:
	// 形状タイプ
	ColliderShape shape_ = ColliderShape::kSphere;

	// 各形状のパラメータ
	Sphere sphere_;
	AABB aabb_;
};

namespace Collision {
	// 汎用衝突判定（コライダー同士）
	bool CheckCollision(Collider* c1, Collider* c2);

	// 各組み合わせの衝突判定（純粋な形状と位置による判定）
	bool CheckCollision(const Sphere& s1, const Vector3& pos1, const Sphere& s2, const Vector3& pos2);
	bool CheckCollision(const AABB& a1, const Vector3& pos1, const AABB& a2, const Vector3& pos2);
	bool CheckCollision(const AABB& aabb, const Vector3& aabbPos, const Sphere& sphere, const Vector3& spherePos);

	// 順序違いをオーバーロードで吸収
	inline bool CheckCollision(const Sphere& sphere, const Vector3& spherePos, const AABB& aabb, const Vector3& aabbPos) {
		return CheckCollision(aabb, aabbPos, sphere, spherePos);
	}
}
