#pragma once

#include "IScene.h"
#include "WorldTransform.h"
#include "LevelData.h"

#include <memory>
#include <vector>
#include <list>

class Object3d;
class Camera;
class DebugCamera;
class RailCameraController;
class Player;
class Level;
class Skydome;
class EnemyBase;
class Collider;
class Shockwave;
class Shake;
class LockOn;
class Goal;
class Sprite;
class CityBackground;
class GameOverUI;
enum class DroneFlightPattern;

class GamePlayScene : public IScene {
public:
	enum class Phase {
		kLeady,    // スタート準備
		kPlay,     // ゲームプレイ
		kClear,    // クリア
		kGameOver, // ゲームオーバー
	};

	Phase phase_ = Phase::kLeady;
public:
	GamePlayScene();
	~GamePlayScene();

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

private:
	// ------------------------------------
	// レベル
	// ------------------------------------

	std::unique_ptr<Level> level_;

	// ------------------------------------
	// 自機
	// ------------------------------------

	std::unique_ptr<Player> player_;
	// モデル
	std::unique_ptr<Object3d> playerModel_;

	// ------------------------------------
	// ロックオン
	// ------------------------------------

	std::unique_ptr<LockOn> lockOn_;

	// ------------------------------------
	// カメラ
	// ------------------------------------

	std::unique_ptr<Camera> camera_;

	// レールカメラ
	std::unique_ptr<RailCameraController> railCamera_;

	// デバッグカメラ
	std::unique_ptr<DebugCamera> debugCamera_;
	bool useDebugCamera_ = false;

	// ------------------------------------
	// 背景 (黒スプライト & サイバー都市)
	// ------------------------------------
	std::unique_ptr<Sprite> backgroundSprite_;
	std::unique_ptr<CityBackground> cityBackground_;

	// ------------------------------------
	// 天球
	// ------------------------------------

	std::unique_ptr<Skydome> skydome_;

	// モデル
	std::unique_ptr<Object3d> skydomeModel_;

	// ------------------------------------
	// 敵
	// ------------------------------------

	std::unique_ptr<Object3d> enemyModel_;
	std::list<std::unique_ptr<EnemyBase>> enemies_;
	std::vector<LevelData::SpawnerData> pendingEnemies_;

	// ------------------------------------
	// エフェクト
	// ------------------------------------

	std::list<std::unique_ptr<Shockwave>> shockwaves_;
	std::unique_ptr<Shake> shake_;

	// ------------------------------------
	// ゴール
	// ------------------------------------
	std::unique_ptr<Goal> goal_;
	bool isGoalReached_ = false;

	// ------------------------------------
	// UI
	// ------------------------------------

	// プレイヤーのHP
	std::unique_ptr<Sprite> uiPlayerHp_;

	// スコア
	int score_ = 0;
	int prevScore_ = -1;

	// スコアUI
	static constexpr int kMaxScoreDigits = 5;
	std::vector<std::unique_ptr<Sprite>> uiScoreDigits_;

	// ------------------------------------
	// フェードアウト
	// ------------------------------------
	std::unique_ptr<Sprite> fadeSprite_;
	float fadeAlpha_ = 0.0f;
	float gameOverWaitTimer_ = 0.0f;
	float clearWaitTimer_ = 0.0f;
	static constexpr float kGameOverFadeDuration = 1.0f; // 破壊演出後のフェードアウト時間（秒）
	static constexpr float kClearFadeDuration = 1.5f;    // ゴール到達後のフェードアウト時間（秒）
	static constexpr float kClearWaitDuration = 0.5f;    // フェード完了後の余韻待機時間（秒）

	// ------------------------------------
	// ゲームオーバー演出UI
	// ------------------------------------
	enum class GameOverStep {
		kPlayerDisappearing, // 自機の消滅演出中（ディゾルブ＋破片散乱）
		kGameOverUI,         // 消滅完了後：背景暗転＋GAMEOVER文字＋グリッチ
	};
	GameOverStep gameOverStep_ = GameOverStep::kPlayerDisappearing;
	float playerDisappearTimer_ = 0.0f;
	static constexpr float kPlayerDisappearPostWait = 0.40f; // 消滅完了後の余韻待機時間（秒）
	std::unique_ptr<GameOverUI> gameOverUI_;
	bool isPlayerDestroyEffectTriggered_ = false; // 撃破時演出発生済みフラグ

	// ------------------------------------
	// ステージ開始カメラ演出 (Phase::kLeady)
	// ------------------------------------
	float introCameraTimer_ = 0.0f;
	float introCameraDuration_ = 3.8f; // カメラ移動時間（秒）
	float playerMaterializeLeadTime_ = 1.2f; // カメラ移動完了の何秒前にPlayer生成を開始するか（秒）
	float playerMaterializeTimer_ = 0.0f; // プレイヤー出現・形成タイマー
	float playerMaterializeDuration_ = 1.4f; // プレイヤー形成時間（秒）
	static constexpr float kPlayerMaterializePostWait = 0.25f; // 形成完了後の余韻待機時間（秒）
	Vector3 introStartOffset_ = { 65.0f, 55.0f, 45.0f }; // ゴールからのオフセット（ゴール斜め右上空から全景を俯瞰）
	Vector3 introSideOffset_ = { 80.0f, 40.0f, 20.0f };  // ステージ中央横の旋回オフセット
	Vector3 introApproachOffset_ = { 35.0f, 15.0f, -25.0f }; // スタート後方アプローチオフセット
	float introStartFov_ = 0.65f; // 開始時FOV（広角）
	float introEndFov_ = 0.45f;   // 通常時FOV
	bool playerMaterializedEffectTriggered_ = false; // 形成完了エフェクト発生済みフラグ

private:

	// ステージ開始時のイントロカメラ演出更新
	void UpdateIntroCamera();

	// ドローン編隊の生成
	void SpawnDroneFormation(const Vector3& basePos, DroneFlightPattern pattern, int count = 4);

	// 全ての衝突判定をチェック
	void CheckAllCollisions();

	// フェーズチェンジ
	void ChangePhase(Phase nextPhase);

	// ImGuiの更新
	void UpdateImGui();
};

