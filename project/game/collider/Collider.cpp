#include "Collider.h"
#include <MathUtility.h>
#include <algorithm>

using namespace MathUtility;

namespace Collision {

	bool CheckCollision(Collider* c1, Collider* c2) {
		ColliderShape shape1 = c1->GetShape();
		ColliderShape shape2 = c2->GetShape();

		if (shape1 == ColliderShape::kSphere && shape2 == ColliderShape::kSphere) {
			// 球と球
			return CheckCollision(c1->GetSphere(), c1->GetWorldPosition(), c2->GetSphere(), c2->GetWorldPosition());
		}
		else if (shape1 == ColliderShape::kAABB && shape2 == ColliderShape::kAABB) {
			// AABBとAABB
			return CheckCollision(c1->GetAABB(), c1->GetWorldPosition(), c2->GetAABB(), c2->GetWorldPosition());
		}
		else {
			// AABBと球
			Collider* aabb = (shape1 == ColliderShape::kAABB) ? c1 : c2;
			Collider* sphere = (shape1 == ColliderShape::kSphere) ? c1 : c2;
			return CheckCollision(aabb->GetAABB(), aabb->GetWorldPosition(), sphere->GetSphere(), sphere->GetWorldPosition());
		}
	}

	bool CheckCollision(const Sphere& s1, const Vector3& pos1, const Sphere& s2, const Vector3& pos2) {
		// 中心点同士の距離の2乗を計算
		Vector3 diff = pos2 - pos1;
		float distSq = Dot(diff, diff);
		
		// 半記の和の2乗を計算
		float radiusSum = s1.radius + s2.radius;
		float radiusSumSq = radiusSum * radiusSum;

		return distSq <= radiusSumSq;
	}

	bool CheckCollision(const AABB& a1, const Vector3& pos1, const AABB& a2, const Vector3& pos2) {
		// 各自のMin/Maxを計算
		Vector3 min1 = pos1 - a1.size * 0.5f;
		Vector3 max1 = pos1 + a1.size * 0.5f;
		Vector3 min2 = pos2 - a2.size * 0.5f;
		Vector3 max2 = pos2 + a2.size * 0.5f;

		// 各軸で重なりがあるか確認
		return (min1.x <= max2.x && max1.x >= min2.x) &&
		       (min1.y <= max2.y && max1.y >= min2.y) &&
		       (min1.z <= max2.z && max1.z >= min2.z);
	}

	bool CheckCollision(const AABB& aabb, const Vector3& aabbPos, const Sphere& sphere, const Vector3& spherePos) {
		// AABBのMin/Maxを計算
		Vector3 min = aabbPos - aabb.size * 0.5f;
		Vector3 max = aabbPos + aabb.size * 0.5f;

		// AABB上で、球の中心に最も近い点をクランプ処理で求める
		Vector3 closestPoint;
		closestPoint.x = std::clamp(spherePos.x, min.x, max.x);
		closestPoint.y = std::clamp(spherePos.y, min.y, max.y);
		closestPoint.z = std::clamp(spherePos.z, min.z, max.z);

		// 最も近い点と、球の中心との距離の2乗を計算
		Vector3 diff = spherePos - closestPoint;
		float distSq = Dot(diff, diff);

		// 距離が半径以下なら衝突
		float radiusSq = sphere.radius * sphere.radius;

		return distSq <= radiusSq;
	}

} // namespace Collision

Vector3 Collider::GetAABBMin() { return GetWorldPosition() - aabb_.size * 0.5f; }

Vector3 Collider::GetAABBMax() { return GetWorldPosition() + aabb_.size * 0.5f; }

