#pragma once

#include <memory>
#include <list>
#include <vector>

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
	const std::list<EnemyBase*>& GetTargets() const { return targets_; }
	void ClearTargets() { targets_.clear(); }

private:
	std::vector<std::unique_ptr<Sprite>> sprites_;

	// ロックオンの限界値（スクリーン上のピクセル距離）
	static constexpr float kDistanceLockOn = 50.0f;
	// 描画サイズ
	static constexpr Vector3 kReticleDrawSize = {0.2f, 0.2f, 0.2f};
	// 最大ロックオン数
	static constexpr size_t kMaxLockOnTargets = 5;

	// ロックオン対象
	std::list<EnemyBase*> targets_;
};

