#pragma once

#include <memory>
#include <list>

#include <Vector3.h>

class Sprite;
class Camera;
class EnemyBase;
class Player;

class LockOn {
public:
	LockOn();
	~LockOn();

	void Initialize();

	void Update(Player* player, std::list<EnemyBase*>& enemies, const Camera* camera);

	void Draw();

public:
	EnemyBase* GetTarget()const { return target_; }

private:
	std::unique_ptr<Sprite> sprite_;

	// ロックオンの限界値（スクリーン上のピクセル距離）
	static constexpr float kDistanceLockOn = 50.0f;
	// 描画サイズ
	static constexpr Vector3 kReticleDrawSize = {0.4f, 0.4f, 0.4f};

	// ロックオン対象
	EnemyBase* target_ = nullptr;
};

