#pragma once

#include <memory>
#include <list>
#include <vector>

#include <Vector3.h>
#include <Vector4.h>

class Camera;
class EnemyBase;
class Player;
class WireframeObject;

class LockOn {
public:
	// ロックオン対象ごとの演出情報を保持する構造体
	struct TargetInfo {
		EnemyBase* enemy = nullptr;

		// スタイリッシュ＆ド派手な多層ワイヤーフレームパーツ
		std::unique_ptr<WireframeObject> outerBracket;  // 外枠スリムブラケット [ ]
		std::unique_ptr<WireframeObject> middleHexagon; // 中間逆回転ヘキサゴン（六角形）
		std::unique_ptr<WireframeObject> segmentArc;    // 内側3分割セグメントアーク（高速回転円弧）
		std::unique_ptr<WireframeObject> centerCross;   // 中心オープン型精密クロスヘア
		std::unique_ptr<WireframeObject> shockwaveRing1;// 第1展開パルスリング
		std::unique_ptr<WireframeObject> shockwaveRing2;// 第2展開パルスリング
		std::unique_ptr<WireframeObject> dataLinkLine;  // 自機〜敵を繋ぐサイバーレーザー線

		// 演出用アニメーションパラメータ
		float lockOnTime = 0.0f;    // ロックオン開始からの経過時間（秒）
		float shockwaveTime = 0.0f; // 衝撃波の経過時間（秒）
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
	static constexpr float kDistanceLockOn = 60.0f;
	// 基準スケール（引き締まったコンパクトサイズ）
	static constexpr float kBaseWireframeSize = 0.95f;
	// 最大ロックオン数
	static constexpr size_t kMaxLockOnTargets = 5;

	// ロックオン対象
	std::list<EnemyBase*> targets_;

	// 演出管理用のターゲットリスト
	std::vector<TargetInfo> targetInfos_;

private:
	// スタイリッシュメッシュ作成ヘルパー
	void CreateSlimBracketMesh(WireframeObject* obj, float size, float cornerLen);
	void CreateSegmentArcMesh(WireframeObject* obj, float radius, int segments = 3, float arcRatio = 0.72f);
	void CreateCenterCrossMesh(WireframeObject* obj, float gapRadius, float length);
};
