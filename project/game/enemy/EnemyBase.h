#pragma once

#include <Vector3.h>

#include "WorldTransform.h"
#include "Collider.h"

class Object3d;
class Camera;

class EnemyBase : public Collider {
public:
	virtual ~EnemyBase() = default;

	virtual void Initialize(Object3d* model, Camera* camera, const Vector3& pos);
	virtual void Update();
	virtual void Draw();

	// コライダーの仮想関数をオーバーライド
	virtual void OnCollision() override;
	virtual Vector3 GetWorldPosition() override;

public:
	const WorldTransform& GetWorldTransform() const { return worldTransform_; }
	WorldTransform& GetWorldTransform() { return worldTransform_; }

	// 生存確認
	bool IsAlive() const { return isAlive_; }

protected:
	// ワールド変換データ
	WorldTransform worldTransform_;

	// モデル
	Object3d* model_ = nullptr;

	// カメラ
	Camera* camera_ = nullptr;

	// 生存フラグ
	bool isAlive_ = true;
};

