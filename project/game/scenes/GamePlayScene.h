#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "SocketServer.h"
#include "IScene.h"

#include <memory>
#include <vector>
#include <Windows.h>
#include <filesystem>

#include "ReplayManager.h"
#include "DebugState.h"

class Object3d;
class Camera;
struct ImGuiContext;

class GamePlayScene : public IScene {
public:
	GamePlayScene();
	~GamePlayScene() override;

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void Finalize() override;

private:
	// DLL ホットリロードのヘルパー関数
	void LoadDebugUI();
	void UnloadDebugUI();
	void UpdateDLL();

	// 状態転送用ヘルパー
	void CaptureStateToVars();
	void ApplyStateFromVars();
	void RecordFrameState();
	void RollbackToFrameState(int frame);

	// シミュレータの描画
	void DrawDataFlowSimulator();

private:
	// モデル
	std::unique_ptr<Object3d> obj_;
	std::vector<std::unique_ptr<Object3d>> objects_;

	// カメラ
	std::unique_ptr<Camera> camera_;

	// 糸（スレッド）表示用球体プール
	std::vector<std::unique_ptr<Object3d>> threadSpherePool_;

	// 敵オブジェクトのリスト (オブジェクトを特定しやすくするため個別に保持)
	std::vector<std::unique_ptr<Object3d>> enemies_;

	// --- デバッグ＆連携システム用メンバ ---
	ReplayManager replayManager_;
	SocketServer socketServer_;
	DebugState debugState_;
	HMODULE debugDll_ = nullptr;
	std::filesystem::file_time_type dllLastWriteTime_;
	typedef void (*DrawDebugUIFunc)(ImGuiContext*, DebugState*);
	DrawDebugUIFunc drawDebugUI_ = nullptr;

	// DLLに渡すフラット配列＆プリミティブデータ
	float playerPos_[3] = {0};
	float playerRot_[3] = {0};
	float playerColor_[4] = {1.0f, 1.0f, 1.0f, 1.0f};

	int threadNodeCount_ = 0;
	float threadNodes_[300] = {0}; // 最大 100 ノード * 3

	int enemyCount_ = 0;
	float enemyPositions_[15] = {0}; // 最大 5 敵 * 3
	float enemyRotations_[15] = {0}; // 最大 5 敵 * 3
	float enemyHPs_[5] = {100.0f, 100.0f, 100.0f, 100.0f, 100.0f};

	bool isPlayback_ = false;
	int playbackFrame_ = 0;
	int totalFrames_ = 0;

	bool isSocketSyncEnabled_ = true;
	bool isSocketConnected_ = false;
	int socketPort_ = 12345;

	bool isBugTriggered_ = false;
	char bugMessage_[256] = "";
	bool triggerBugNow_ = false;
	bool saveReplayNow_ = false;
	bool loadReplayNow_ = false;

	// 自律移動のためのタイマーやパラメータなど
	float enemyTimer_ = 0.0f;
	float threadSpawnTimer_ = 0.0f;
	std::vector<Vector3> activeThreadNodes_; // 実行時用動的ノードリスト
	std::vector<Vector3> enemyInitialPositions_; // 敵の初期位置（基準位置）
};
