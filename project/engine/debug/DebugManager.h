#pragma once

#ifdef USE_IMGUI

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <Xinput.h>

#include <Vector3.h>

// 前方宣言
class Object3d;
class Input;

//================================================================================
// デバッグ変数の型タグ
//================================================================================

enum class DebugVarType {
	kFloat,
	kInt,
	kBool,
	kVec3
};

//================================================================================
// デバッグ変数エントリ
//================================================================================

struct DebugVarEntry {
	std::string   name;   // 変数名（例: "HP"）
	DebugVarType  type;   // 型
	void*         ptr;    // 実際の変数へのポインタ
};

//================================================================================
// 入力スナップショット（1フレーム分）
//================================================================================

struct InputSnapshot {
	BYTE          keys[256] = {};
	DIMOUSESTATE  mouse = {};
	XINPUT_STATE  gamepad = {};
};

//================================================================================
// オブジェクトスナップショット（1オブジェクト分）
//================================================================================

struct ObjectSnapshot {
	std::string id;
	std::string modelName;
	Vector3     position;
	Vector3     rotation;
	Vector3     scale;
};

//================================================================================
// 関数トレースエントリ
//================================================================================

struct TraceEntry {
	std::string functionName;
	int         frame = 0;
	float       timeMs = 0.0f;
};

//================================================================================
// 1フレーム分の全データ
//================================================================================

struct FrameData {
	int                          frameNumber = 0;
	InputSnapshot                input;
	std::vector<ObjectSnapshot>  objects;
	std::vector<TraceEntry>      traces;
	// デバッグ変数のスナップショット（グループ名 → 変数名 → 値文字列）
	std::unordered_map<std::string,
		std::unordered_map<std::string, std::string>> debugVarSnapshots;
};

//================================================================================
// コマンド（Blenderからの命令）
//================================================================================

struct DebugCommand {
	std::string type;     // "seek", "pause", "resume", "export_state", "inject_state"
	int         frame = 0;
	std::string path;
};

//================================================================================
// DebugManager クラス
//================================================================================

class DebugManager {
public:
	//================================================================================
	// シングルトン
	//================================================================================

	static DebugManager* GetInstance();

	// コピー・代入禁止
	DebugManager(const DebugManager&) = delete;
	DebugManager& operator=(const DebugManager&) = delete;

	//================================================================================
	// ライフサイクル
	//================================================================================

	/// <summary>
	/// 初期化（TCPサーバー起動）
	/// </summary>
	void Initialize();

	/// <summary>
	/// 毎フレーム更新（記録・送信・コマンド処理）
	/// </summary>
	void Update();

	/// <summary>
	/// 終了処理（ソケット・スレッドクリーンアップ）
	/// </summary>
	void Finalize();

	//================================================================================
	// オブジェクト管理
	//================================================================================

	/// <summary>
	/// Object3dを追跡登録し、一意のIDを割り当てる
	/// </summary>
	/// <param name="obj">登録するオブジェクト</param>
	/// <returns>割り当てられたID</returns>
	std::string RegisterObject(Object3d* obj);

	/// <summary>
	/// Object3dの登録を解除する
	/// </summary>
	/// <param name="obj">解除するオブジェクト</param>
	void UnregisterObject(Object3d* obj);

	//================================================================================
	// デバッグ変数監視（ゲームクラスから直接呼ぶ）
	//================================================================================

	/// <summary>
	/// float変数を監視登録
	/// </summary>
	void WatchFloat(const std::string& group, const std::string& name, float* ptr);

	/// <summary>
	/// int変数を監視登録
	/// </summary>
	void WatchInt(const std::string& group, const std::string& name, int* ptr);

	/// <summary>
	/// bool変数を監視登録
	/// </summary>
	void WatchBool(const std::string& group, const std::string& name, bool* ptr);

	/// <summary>
	/// Vector3変数を監視登録
	/// </summary>
	void WatchVec3(const std::string& group, const std::string& name, Vector3* ptr);

	/// <summary>
	/// 指定グループの全変数を登録解除
	/// </summary>
	void UnwatchAll(const std::string& group);

	//================================================================================
	// 関数トレース
	//================================================================================

	/// <summary>
	/// 現フレームに関数トレースを記録
	/// </summary>
	void TraceFunction(const char* functionName);

	//================================================================================
	// 状態確認
	//================================================================================

	/// <summary>
	/// ゲームが一時停止中かどうか
	/// </summary>
	bool IsPaused() const { return isPaused_; }

	/// <summary>
	/// 現在のフレーム番号
	/// </summary>
	int GetCurrentFrame() const { return currentFrame_; }

private:
	//================================================================================
	// コンストラクタ・デストラクタ（シングルトン用）
	//================================================================================

	DebugManager() = default;
	~DebugManager() = default;

	//================================================================================
	// ソケット通信
	//================================================================================

	/// <summary>
	/// ソケットサーバーの初期化
	/// </summary>
	void InitializeSocket();

	/// <summary>
	/// 受信スレッドのメインループ
	/// </summary>
	void ReceiveThreadFunc();

	/// <summary>
	/// BlenderにJSONメッセージを送信
	/// </summary>
	void SendToBlender(const std::string& json);

	/// <summary>
	/// 受信データを解析してコマンドに変換
	/// </summary>
	void ParseReceivedData(const std::string& data);

	//================================================================================
	// コマンド処理
	//================================================================================

	/// <summary>
	/// キューのコマンドを処理
	/// </summary>
	void ProcessCommands();

	/// <summary>
	/// seekコマンドの処理（巻き戻し）
	/// </summary>
	void HandleSeek(int targetFrame);

	/// <summary>
	/// 状態エクスポート
	/// </summary>
	void HandleExportState(const std::string& path);

	/// <summary>
	/// 状態インジェクト
	/// </summary>
	void HandleInjectState(const std::string& path);

	//================================================================================
	// 記録
	//================================================================================

	/// <summary>
	/// 現フレームのデータを記録
	/// </summary>
	void RecordFrame();

	/// <summary>
	/// 現在のフレームデータをJSONに変換して送信
	/// </summary>
	void SendFrameUpdate();

	/// <summary>
	/// 全デバッグ変数の現在値をスナップショット化
	/// </summary>
	std::unordered_map<std::string,
		std::unordered_map<std::string, std::string>> SnapshotDebugVars() const;

	/// <summary>
	/// デバッグ変数エントリの値を文字列に変換
	/// </summary>
	std::string VarToString(const DebugVarEntry& entry) const;

	/// <summary>
	/// デバッグ変数のスナップショットからゲーム変数を復元
	/// </summary>
	void RestoreDebugVars(const std::unordered_map<std::string,
		std::unordered_map<std::string, std::string>>& snapshot);

	/// <summary>
	/// 文字列からデバッグ変数に値を書き戻す
	/// </summary>
	void StringToVar(const DebugVarEntry& entry, const std::string& value);

	//================================================================================
	// メンバ変数
	//================================================================================

	// フレームカウンタ
	int currentFrame_ = 0;

	// 一時停止フラグ
	std::atomic<bool> isPaused_ = false;

	// 登録オブジェクト
	std::vector<Object3d*> trackedObjects_;
	std::unordered_map<std::string, int> modelNameCounters_; // ID生成用カウンタ

	// デバッグ変数（グループ名 → エントリリスト）
	std::unordered_map<std::string, std::vector<DebugVarEntry>> debugVarGroups_;

	// 関数トレース（現フレーム用の一時バッファ）
	std::vector<TraceEntry> currentTraces_;

	// リングバッファ
	static constexpr int kMaxFrames = 10800;        // 約3分 @ 60fps
	static constexpr int kSnapshotInterval = 30;    // 30フレームごと
	static constexpr int kMaxSnapshots = 360;       // kMaxFrames / kSnapshotInterval

	std::deque<FrameData> frameBuffer_;
	std::deque<FrameData> snapshotBuffer_;           // 定期的な完全スナップショット

	// ソケット通信
	SOCKET listenSocket_ = INVALID_SOCKET;
	SOCKET clientSocket_ = INVALID_SOCKET;
	std::thread receiveThread_;
	std::atomic<bool> isRunning_ = false;

	// コマンドキュー（スレッドセーフ）
	std::mutex commandMutex_;
	std::vector<DebugCommand> commandQueue_;

	// 送信用ミューテックス
	std::mutex sendMutex_;

	// 受信バッファ
	std::string receiveBuffer_;

	// フレームタイマー（トレース用）
	float frameStartTimeMs_ = 0.0f;
};

//================================================================================
// 関数トレースマクロ
//================================================================================

#define DEBUG_TRACE_FUNC() \
	DebugManager::GetInstance()->TraceFunction(__FUNCTION__)

#else // USE_IMGUI が無い場合

#define DEBUG_TRACE_FUNC() ((void)0)

#endif // USE_IMGUI
