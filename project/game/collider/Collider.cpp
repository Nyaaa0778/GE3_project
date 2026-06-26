#include "Collider.h"

#include <MathUtility.h>

#include <algorithm>
#include <cmath>

using namespace MathUtility;

namespace Collision {

	bool CheckCollision(Collider* c1, Collider* c2) {
		ColliderShape shape1 = c1->GetShape();
		ColliderShape shape2 = c2->GetShape();

		// 移動前と移動後の位置を取得
		Vector3 p1_start = c1->GetPrevWorldPosition();
		Vector3 p1_end = c1->GetWorldPosition();
		Vector3 p2_start = c2->GetPrevWorldPosition();
		Vector3 p2_end = c2->GetWorldPosition();

		Vector3 diff1 = p1_end - p1_start;
		Vector3 diff2 = p2_end - p2_start;

		float dist1 = Length(diff1);
		float dist2 = Length(diff2);

		// 各コライダーのサイズを算出
		float size1 = (shape1 == ColliderShape::kSphere) ? c1->GetSphere().radius : Length(c1->GetAABB().size) * 0.5f;
		float size2 = (shape2 == ColliderShape::kSphere) ? c2->GetSphere().radius : Length(c2->GetAABB().size) * 0.5f;

		// 移動距離がサイズに対して十分大きい場合、すり抜け対策（CCD）を有効にする
		bool useCCD1 = dist1 > size1 * 0.5f;
		bool useCCD2 = dist2 > size2 * 0.5f;

		if (useCCD1 || useCCD2) {
			// 相対移動ベクトルから必要な分割数を算出
			Vector3 relDiff = diff1 - diff2;
			float relDist = Length(relDiff);

			float minSize = (std::min)(size1, size2);
			float step = minSize * 0.8f;
			if (step <= 0.0f) step = 0.1f;

			int steps = static_cast<int>(std::ceil(relDist / step));
			if (steps < 1) steps = 1;

			// 時間 t [0, 1] でそれぞれの位置を線形補間しながら判定
			for (int i = 0; i <= steps; ++i) {
				float t = static_cast<float>(i) / static_cast<float>(steps);
				Vector3 pos1 = p1_start + diff1 * t;
				Vector3 pos2 = p2_start + diff2 * t;

				bool isHit = false;
				if (shape1 == ColliderShape::kSphere && shape2 == ColliderShape::kSphere) {
					isHit = CheckCollision(c1->GetSphere(), pos1, c2->GetSphere(), pos2);
				}
				else if (shape1 == ColliderShape::kAABB && shape2 == ColliderShape::kAABB) {
					isHit = CheckCollision(c1->GetAABB(), pos1, c2->GetAABB(), pos2);
				}
				else {
					Collider* aabb = (shape1 == ColliderShape::kAABB) ? c1 : c2;
					Vector3 aabbPos = (shape1 == ColliderShape::kAABB) ? pos1 : pos2;
					Collider* sphere = (shape1 == ColliderShape::kSphere) ? c1 : c2;
					Vector3 spherePos = (shape1 == ColliderShape::kSphere) ? pos1 : pos2;
					isHit = CheckCollision(aabb->GetAABB(), aabbPos, sphere->GetSphere(), spherePos);
				}

				if (isHit) {
					return true;
				}
			}
			return false;
		}

		// どちらも高速移動していない場合は通常の1点判定
		if (shape1 == ColliderShape::kSphere && shape2 == ColliderShape::kSphere) {
			// 球と球
			return CheckCollision(c1->GetSphere(), p1_end, c2->GetSphere(), p2_end);
		}
		else if (shape1 == ColliderShape::kAABB && shape2 == ColliderShape::kAABB) {
			// AABBとAABB
			return CheckCollision(c1->GetAABB(), p1_end, c2->GetAABB(), p2_end);
		}
		else {
			// AABBと球
			Collider* aabb = (shape1 == ColliderShape::kAABB) ? c1 : c2;
			Vector3 aabbPos = (shape1 == ColliderShape::kAABB) ? p1_end : p2_end;
			Collider* sphere = (shape1 == ColliderShape::kSphere) ? c1 : c2;
			Vector3 spherePos = (shape1 == ColliderShape::kSphere) ? p1_end : p2_end;
			return CheckCollision(aabb->GetAABB(), aabbPos, sphere->GetSphere(), spherePos);
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

