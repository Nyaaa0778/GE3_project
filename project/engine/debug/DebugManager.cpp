#ifdef USE_IMGUI

#include "DebugManager.h"
#include "Object3d.h"
#include "Input.h"

#include <json.hpp>

#include <chrono>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cassert>

using json = nlohmann::json;

//================================================================================
// シングルトン
//================================================================================

DebugManager* DebugManager::GetInstance() {
	static DebugManager instance;
	return &instance;
}

//================================================================================
// ライフサイクル
//================================================================================

void DebugManager::Initialize() {
	currentFrame_ = 0;
	isPaused_ = false;
	isRunning_ = true;

	InitializeSocket();
}

void DebugManager::Update() {
	// コマンドキューを処理
	ProcessCommands();

	// 一時停止中は記録・送信しない
	if (isPaused_) {
		return;
	}

	// フレーム開始時刻を記録（トレース用）
	auto now = std::chrono::high_resolution_clock::now();
	frameStartTimeMs_ = static_cast<float>(
		std::chrono::duration_cast<std::chrono::microseconds>(
			now.time_since_epoch()).count()) / 1000.0f;

	// 現フレームのデータを記録
	RecordFrame();

	// Blenderにフレームデータを送信
	SendFrameUpdate();

	// フレームカウンタを進める
	currentFrame_++;

	// 関数トレースバッファをクリア（次フレーム用）
	currentTraces_.clear();
}

void DebugManager::Finalize() {
	// スレッドとソケットを停止
	isRunning_ = false;

	// ソケットを閉じて受信スレッドのブロックを解除
	if (clientSocket_ != INVALID_SOCKET) {
		closesocket(clientSocket_);
		clientSocket_ = INVALID_SOCKET;
	}
	if (listenSocket_ != INVALID_SOCKET) {
		closesocket(listenSocket_);
		listenSocket_ = INVALID_SOCKET;
	}

	// スレッドの終了を待つ
	if (receiveThread_.joinable()) {
		receiveThread_.join();
	}

	// Winsockクリーンアップ
	WSACleanup();

	// バッファクリア
	frameBuffer_.clear();
	snapshotBuffer_.clear();
	trackedObjects_.clear();
	debugVarGroups_.clear();
	commandQueue_.clear();
}

//================================================================================
// オブジェクト管理
//================================================================================

std::string DebugManager::RegisterObject(Object3d* obj) {
	// 既に登録されていないか確認
	for (const auto* tracked : trackedObjects_) {
		if (tracked == obj) {
			return obj->GetId();
		}
	}

	// モデル名からユニークIDを生成
	const std::string& modelName = obj->GetModelName();
	int count = modelNameCounters_[modelName]++;
	std::string id = modelName + "_" + std::to_string(count);

	trackedObjects_.push_back(obj);
	return id;
}

void DebugManager::UnregisterObject(Object3d* obj) {
	auto it = std::find(trackedObjects_.begin(), trackedObjects_.end(), obj);
	if (it != trackedObjects_.end()) {
		trackedObjects_.erase(it);
	}
}

//================================================================================
// デバッグ変数監視
//================================================================================

void DebugManager::WatchFloat(const std::string& group, const std::string& name, float* ptr) {
	auto& entries = debugVarGroups_[group];
	for (auto& entry : entries) {
		if (entry.name == name) {
			entry.ptr = ptr;
			return;
		}
	}
	entries.push_back({name, DebugVarType::kFloat, ptr});
}

void DebugManager::WatchInt(const std::string& group, const std::string& name, int* ptr) {
	auto& entries = debugVarGroups_[group];
	for (auto& entry : entries) {
		if (entry.name == name) {
			entry.ptr = ptr;
			return;
		}
	}
	entries.push_back({name, DebugVarType::kInt, ptr});
}

void DebugManager::WatchBool(const std::string& group, const std::string& name, bool* ptr) {
	auto& entries = debugVarGroups_[group];
	for (auto& entry : entries) {
		if (entry.name == name) {
			entry.ptr = ptr;
			return;
		}
	}
	entries.push_back({name, DebugVarType::kBool, ptr});
}

void DebugManager::WatchVec3(const std::string& group, const std::string& name, Vector3* ptr) {
	auto& entries = debugVarGroups_[group];
	for (auto& entry : entries) {
		if (entry.name == name) {
			entry.ptr = ptr;
			return;
		}
	}
	entries.push_back({name, DebugVarType::kVec3, ptr});
}

void DebugManager::UnwatchAll(const std::string& group) {
	debugVarGroups_.erase(group);
}

//================================================================================
// 関数トレース
//================================================================================

void DebugManager::TraceFunction(const char* functionName) {
	auto now = std::chrono::high_resolution_clock::now();
	float currentTimeMs = static_cast<float>(
		std::chrono::duration_cast<std::chrono::microseconds>(
			now.time_since_epoch()).count()) / 1000.0f;

	TraceEntry entry;
	entry.functionName = functionName;
	entry.frame = currentFrame_;
	entry.timeMs = currentTimeMs - frameStartTimeMs_;

	currentTraces_.push_back(entry);
}

//================================================================================
// ソケット通信
//================================================================================

void DebugManager::InitializeSocket() {
	// Winsock初期化
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		return; // 初期化失敗時は通信なしで動作
	}

	// リスニングソケットの作成
	listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket_ == INVALID_SOCKET) {
		WSACleanup();
		return;
	}

	// SO_REUSEADDR を設定（再起動時のポート占有を防止）
	int opt = 1;
	setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR,
		reinterpret_cast<const char*>(&opt), sizeof(opt));

	// ローカルホスト（127.0.0.1）のみにバインド
	sockaddr_in serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(9999);
	serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	result = bind(listenSocket_, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
	if (result == SOCKET_ERROR) {
		closesocket(listenSocket_);
		listenSocket_ = INVALID_SOCKET;
		WSACleanup();
		return;
	}

	// リスニング開始
	result = listen(listenSocket_, 1);
	if (result == SOCKET_ERROR) {
		closesocket(listenSocket_);
		listenSocket_ = INVALID_SOCKET;
		WSACleanup();
		return;
	}

	// 受信スレッドを起動
	receiveThread_ = std::thread(&DebugManager::ReceiveThreadFunc, this);
}

void DebugManager::ReceiveThreadFunc() {
	while (isRunning_) {
		// クライアントが未接続なら接続を待つ
		if (clientSocket_ == INVALID_SOCKET) {
			// selectで接続待ち（タイムアウト付き）
			fd_set readSet;
			FD_ZERO(&readSet);
			FD_SET(listenSocket_, &readSet);

			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = 100000; // 100ms

			int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);
			if (selectResult > 0 && FD_ISSET(listenSocket_, &readSet)) {
				SOCKET newClient = accept(listenSocket_, nullptr, nullptr);
				if (newClient != INVALID_SOCKET) {
					clientSocket_ = newClient;

					// ノンブロッキングモードに設定
					u_long mode = 1;
					ioctlsocket(clientSocket_, FIONBIO, &mode);
				}
			}
			continue;
		}

		// 接続中のクライアントからデータを受信
		char buffer[4096];
		int bytesReceived = recv(clientSocket_, buffer, sizeof(buffer) - 1, 0);

		if (bytesReceived > 0) {
			buffer[bytesReceived] = '\0';
			receiveBuffer_ += buffer;

			// 改行区切りでメッセージを分割
			size_t pos;
			while ((pos = receiveBuffer_.find('\n')) != std::string::npos) {
				std::string message = receiveBuffer_.substr(0, pos);
				receiveBuffer_ = receiveBuffer_.substr(pos + 1);

				if (!message.empty()) {
					ParseReceivedData(message);
				}
			}
		} else if (bytesReceived == 0) {
			// クライアントが切断
			closesocket(clientSocket_);
			clientSocket_ = INVALID_SOCKET;
			receiveBuffer_.clear();
		} else {
			// WSAEWOULDBLOCK は正常（データなし）
			int err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK) {
				closesocket(clientSocket_);
				clientSocket_ = INVALID_SOCKET;
				receiveBuffer_.clear();
			} else {
				// データなし：少し待つ
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	}
}

void DebugManager::SendToBlender(const std::string& jsonStr) {
	std::lock_guard<std::mutex> lock(sendMutex_);

	if (clientSocket_ == INVALID_SOCKET) {
		return;
	}

	std::string message = jsonStr + "\n";
	int totalSent = 0;
	int remaining = static_cast<int>(message.size());

	while (remaining > 0) {
		int sent = send(clientSocket_, message.c_str() + totalSent, remaining, 0);
		if (sent == SOCKET_ERROR) {
			// 送信失敗
			break;
		}
		totalSent += sent;
		remaining -= sent;
	}
}

void DebugManager::ParseReceivedData(const std::string& data) {
	try {
		json j = json::parse(data);

		DebugCommand cmd;
		cmd.type = j.value("type", "");
		cmd.frame = j.value("frame", 0);
		cmd.path = j.value("path", "");

		std::lock_guard<std::mutex> lock(commandMutex_);
		commandQueue_.push_back(cmd);
	} catch (...) {
		// JSON解析に失敗した場合は無視
	}
}

//================================================================================
// コマンド処理
//================================================================================

void DebugManager::ProcessCommands() {
	std::vector<DebugCommand> commands;
	{
		std::lock_guard<std::mutex> lock(commandMutex_);
		commands.swap(commandQueue_);
	}

	for (const auto& cmd : commands) {
		if (cmd.type == "seek") {
			HandleSeek(cmd.frame);
		} else if (cmd.type == "pause") {
			isPaused_ = true;
		} else if (cmd.type == "resume") {
			isPaused_ = false;
			// トレースバッファをクリア（再開時はクリーンな状態から）
			currentTraces_.clear();
		} else if (cmd.type == "export_state") {
			HandleExportState(cmd.path);
		} else if (cmd.type == "inject_state") {
			HandleInjectState(cmd.path);
		}
	}
}

void DebugManager::HandleSeek(int targetFrame) {
	// ターゲットフレームが記録範囲内かチェック
	if (frameBuffer_.empty()) {
		return;
	}

	int oldestFrame = frameBuffer_.front().frameNumber;
	int newestFrame = frameBuffer_.back().frameNumber;

	if (targetFrame < oldestFrame || targetFrame > newestFrame) {
		return; // 範囲外
	}

	// ★ 自動一時停止（復元した状態をゲームロジックが上書きしないように）
	isPaused_ = true;

	// ターゲットフレームのデータを取得
	int targetIndex = targetFrame - oldestFrame;
	if (targetIndex < 0 || targetIndex >= static_cast<int>(frameBuffer_.size())) {
		return;
	}

	const auto& targetData = frameBuffer_[targetIndex];

	// ★ デバッグ変数を復元（Player::pos_ 等のゲームロジック内部状態）
	// これを先にやることで、Object3d復元後にゲーム変数が整合する
	RestoreDebugVars(targetData.debugVarSnapshots);

	// オブジェクトの状態を復元
	for (const auto& objSnap : targetData.objects) {
		for (auto* obj : trackedObjects_) {
			if (obj->GetId() == objSnap.id) {
				obj->SetPosition(objSnap.position);
				obj->SetRotation(objSnap.rotation);
				obj->SetScale(objSnap.scale);
				break;
			}
		}
	}

	// 現在フレームをターゲットに設定してBlenderに送信
	currentFrame_ = targetFrame;
	SendFrameUpdate();

	// ★ resume後に新しいフレーム番号から記録を開始できるようインクリメント
	// これにより、resume後の最初のRecordFrameが重複フレーム番号にならない
	currentFrame_ = targetFrame + 1;

	// ★ ターゲットフレーム以降の記録データを削除（未来のデータを破棄）
	while (!frameBuffer_.empty() && frameBuffer_.back().frameNumber > targetFrame) {
		frameBuffer_.pop_back();
	}
	while (!snapshotBuffer_.empty() && snapshotBuffer_.back().frameNumber > targetFrame) {
		snapshotBuffer_.pop_back();
	}
}

void DebugManager::HandleExportState(const std::string& path) {
	std::string filePath = path.empty() ? "autosave.state" : path;

	json j;
	j["frame"] = currentFrame_;

	// オブジェクト状態
	json objectsJson = json::array();
	for (const auto* obj : trackedObjects_) {
		json objJson;
		objJson["id"] = obj->GetId();
		objJson["model"] = obj->GetModelName();
		objJson["position"] = {obj->GetPosition().x, obj->GetPosition().y, obj->GetPosition().z};
		objJson["rotation"] = {obj->GetRotate().x, obj->GetRotate().y, obj->GetRotate().z};
		objJson["scale"] = {obj->GetScale().x, obj->GetScale().y, obj->GetScale().z};
		objectsJson.push_back(objJson);
	}
	j["objects"] = objectsJson;

	// デバッグ変数
	j["variables"] = json::object();
	for (const auto& [group, entries] : debugVarGroups_) {
		json groupJson;
		for (const auto& entry : entries) {
			groupJson[entry.name] = VarToString(entry);
		}
		j["variables"][group] = groupJson;
	}

	// 入力データバッファ（直近の入力を一部保存）
	json inputsJson = json::array();
	int startFrame = (currentFrame_ > 300) ? currentFrame_ - 300 : 0;
	for (const auto& fd : frameBuffer_) {
		if (fd.frameNumber >= startFrame) {
			json inputJson;
			inputJson["frame"] = fd.frameNumber;
			// キーボード状態をbase64的に保存（簡略化：16進文字列）
			std::string keysHex;
			for (int k = 0; k < 256; ++k) {
				char hex[3];
				snprintf(hex, sizeof(hex), "%02X", fd.input.keys[k]);
				keysHex += hex;
			}
			inputJson["keys"] = keysHex;
			inputJson["buttons"] = fd.input.gamepad.Gamepad.wButtons;
			inputsJson.push_back(inputJson);
		}
	}
	j["inputs"] = inputsJson;

	// ファイルに書き出し
	std::ofstream ofs(filePath);
	if (ofs.is_open()) {
		ofs << j.dump(2);
	}
}

void DebugManager::HandleInjectState(const std::string& path) {
	std::string filePath = path.empty() ? "autosave.state" : path;

	std::ifstream ifs(filePath);
	if (!ifs.is_open()) {
		return;
	}

	try {
		json j;
		ifs >> j;

		// フレームの復元
		currentFrame_ = j.value("frame", 0);

		// オブジェクト状態の復元
		if (j.contains("objects")) {
			for (const auto& objJson : j["objects"]) {
				std::string id = objJson.value("id", "");
				for (auto* obj : trackedObjects_) {
					if (obj->GetId() == id) {
						if (objJson.contains("position")) {
							Vector3 pos;
							pos.x = objJson["position"][0];
							pos.y = objJson["position"][1];
							pos.z = objJson["position"][2];
							obj->SetPosition(pos);
						}
						if (objJson.contains("rotation")) {
							Vector3 rot;
							rot.x = objJson["rotation"][0];
							rot.y = objJson["rotation"][1];
							rot.z = objJson["rotation"][2];
							obj->SetRotation(rot);
						}
						if (objJson.contains("scale")) {
							Vector3 scl;
							scl.x = objJson["scale"][0];
							scl.y = objJson["scale"][1];
							scl.z = objJson["scale"][2];
							obj->SetScale(scl);
						}
						break;
					}
				}
			}
		}

		// 巻き戻し後のフレームデータをBlenderに送信
		SendFrameUpdate();
	} catch (...) {
		// JSONパースに失敗した場合は無視
	}
}

//================================================================================
// 記録
//================================================================================

void DebugManager::RecordFrame() {
	FrameData fd;
	fd.frameNumber = currentFrame_;

	// 入力スナップショットを取得
	Input* input = Input::GetInstance();
	const BYTE* keys = input->GetCurrentKeys();
	memcpy(fd.input.keys, keys, sizeof(fd.input.keys));
	fd.input.mouse = input->GetCurrentMouse();
	fd.input.gamepad = input->GetCurrentGamepad();

	// オブジェクトスナップショットを取得
	for (const auto* obj : trackedObjects_) {
		ObjectSnapshot objSnap;
		objSnap.id = obj->GetId();
		objSnap.modelName = obj->GetModelName();
		objSnap.position = obj->GetPosition();
		objSnap.rotation = obj->GetRotate();
		objSnap.scale = obj->GetScale();
		fd.objects.push_back(objSnap);
	}

	// 関数トレースを取得
	fd.traces = currentTraces_;

	// デバッグ変数のスナップショットを取得
	fd.debugVarSnapshots = SnapshotDebugVars();

	// リングバッファに追加
	frameBuffer_.push_back(fd);
	if (static_cast<int>(frameBuffer_.size()) > kMaxFrames) {
		frameBuffer_.pop_front();
	}

	// 定期的にスナップショットバッファにも保存
	if (currentFrame_ % kSnapshotInterval == 0) {
		snapshotBuffer_.push_back(fd);
		if (static_cast<int>(snapshotBuffer_.size()) > kMaxSnapshots) {
			snapshotBuffer_.pop_front();
		}
	}
}

void DebugManager::SendFrameUpdate() {
	if (clientSocket_ == INVALID_SOCKET) {
		return;
	}

	json j;
	j["type"] = "frame_update";
	j["frame"] = currentFrame_;

	// オブジェクトデータ
	json objectsJson = json::array();
	for (const auto* obj : trackedObjects_) {
		json objJson;
		objJson["id"] = obj->GetId();
		objJson["model"] = obj->GetModelName();
		objJson["position"] = {obj->GetPosition().x, obj->GetPosition().y, obj->GetPosition().z};
		objJson["rotation"] = {obj->GetRotate().x, obj->GetRotate().y, obj->GetRotate().z};
		objJson["scale"] = {obj->GetScale().x, obj->GetScale().y, obj->GetScale().z};
		objectsJson.push_back(objJson);
	}
	j["objects"] = objectsJson;

	// デバッグ変数（グループ別）
	json variablesJson = json::object();
	for (const auto& [group, entries] : debugVarGroups_) {
		json groupJson;
		for (const auto& entry : entries) {
			groupJson[entry.name] = VarToString(entry);
		}
		variablesJson[group] = groupJson;
	}
	j["variables"] = variablesJson;

	// 関数トレース
	json callStackJson = json::array();
	for (const auto& trace : currentTraces_) {
		json traceJson;
		traceJson["func"] = trace.functionName;
		traceJson["frame"] = trace.frame;
		traceJson["time_ms"] = trace.timeMs;
		callStackJson.push_back(traceJson);
	}
	j["call_stack"] = callStackJson;

	SendToBlender(j.dump());
}

std::unordered_map<std::string,
	std::unordered_map<std::string, std::string>> DebugManager::SnapshotDebugVars() const {
	std::unordered_map<std::string,
		std::unordered_map<std::string, std::string>> result;

	for (const auto& [group, entries] : debugVarGroups_) {
		for (const auto& entry : entries) {
			result[group][entry.name] = VarToString(entry);
		}
	}

	return result;
}

std::string DebugManager::VarToString(const DebugVarEntry& entry) const {
	if (!entry.ptr) {
		return "null";
	}

	switch (entry.type) {
	case DebugVarType::kFloat: {
		float val = *static_cast<float*>(entry.ptr);
		char buf[64];
		snprintf(buf, sizeof(buf), "%.3f", val);
		return buf;
	}
	case DebugVarType::kInt: {
		int val = *static_cast<int*>(entry.ptr);
		return std::to_string(val);
	}
	case DebugVarType::kBool: {
		bool val = *static_cast<bool*>(entry.ptr);
		return val ? "true" : "false";
	}
	case DebugVarType::kVec3: {
		Vector3 val = *static_cast<Vector3*>(entry.ptr);
		char buf[128];
		snprintf(buf, sizeof(buf), "(%.3f, %.3f, %.3f)", val.x, val.y, val.z);
		return buf;
	}
	default:
		return "unknown";
	}
}

//================================================================================
// デバッグ変数の復元
//================================================================================

void DebugManager::RestoreDebugVars(const std::unordered_map<std::string,
	std::unordered_map<std::string, std::string>>& snapshot) {
	for (const auto& [group, vars] : snapshot) {
		auto groupIt = debugVarGroups_.find(group);
		if (groupIt == debugVarGroups_.end()) {
			continue;
		}

		for (const auto& [name, value] : vars) {
			for (auto& entry : groupIt->second) {
				if (entry.name == name) {
					StringToVar(entry, value);
					break;
				}
			}
		}
	}
}

void DebugManager::StringToVar(const DebugVarEntry& entry, const std::string& value) {
	if (!entry.ptr || value == "null" || value.empty()) {
		return;
	}

	try {
		switch (entry.type) {
		case DebugVarType::kFloat: {
			*static_cast<float*>(entry.ptr) = std::stof(value);
			break;
		}
		case DebugVarType::kInt: {
			*static_cast<int*>(entry.ptr) = std::stoi(value);
			break;
		}
		case DebugVarType::kBool: {
			*static_cast<bool*>(entry.ptr) = (value == "true");
			break;
		}
		case DebugVarType::kVec3: {
			Vector3& vec = *static_cast<Vector3*>(entry.ptr);
			// "(x, y, z)" 形式をパース
			if (sscanf_s(value.c_str(), "(%f, %f, %f)", &vec.x, &vec.y, &vec.z) != 3) {
				// パース失敗時は何もしない
			}
			break;
		}
		default:
			break;
		}
	} catch (...) {
		// 変換失敗時は無視
	}
}

#endif // USE_IMGUI
