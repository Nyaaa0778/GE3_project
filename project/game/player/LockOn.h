#pragma once

#include <memory>
#include <list>
#include <vector>

#include <Vector3.h>
#include <Vector4.h>

class Sprite;
class Camera;
class EnemyBase;
class Player;

class LockOn {
public:
	// ロックオン対象ごとの演出情報を保持する構造体
	struct TargetInfo {
		EnemyBase* enemy = nullptr;
		std::unique_ptr<Sprite> sprite;

		// 演出用アニメーションパラメータ
		float lockOnTime = 0.0f; // ロックオン開始からの経過時間（秒）

		// 衝撃波（演出用）スプライトと時間
		std::unique_ptr<Sprite> effectSprite;
		float effectTime = 0.0f; // エフェクト開始からの経過時間（秒）
	};

public:
	LockOn();
	~LockOn();

	void Initialize();

	void Update(Player* player, std::list<EnemyBase*>& enemies, const Camera* camera);

	void Draw();

public:
	const std::list<EnemyBase*>& GetTargets() const { return targets_; }
	void ClearTargets();

private:
	// ロックオンの限界値（スクリーン上のピクセル距離）
	static constexpr float kDistanceLockOn = 50.0f;
	// 描画サイズ
	static constexpr Vector3 kReticleDrawSize = {0.2f, 0.2f, 0.2f};
	// 最大ロックオン数
	static constexpr size_t kMaxLockOnTargets = 5;

	// ロックオン対象
	std::list<EnemyBase*> targets_;

	// 演出管理用のターゲットリスト
	std::vector<TargetInfo> targetInfos_;

	// 音声再生用
	uint32_t lockOnSeHandle_ = 0;
	bool isSeLoaded_ = false;
};


