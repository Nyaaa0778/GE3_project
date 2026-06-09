#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

class DebugNetwork {
public:
	static DebugNetwork* GetInstance();
	void Initialize(int port = 12345);
	void Finalize();

	// 受信確認。FRAME:<フレーム番号> が来ていればその数値を返す。なければ -1。
	int UpdateReceive();

private:
	DebugNetwork() = default;
	~DebugNetwork() = default;
	DebugNetwork(const DebugNetwork&) = delete;
	DebugNetwork& operator=(const DebugNetwork&) = delete;

	SOCKET sock_ = INVALID_SOCKET;
	bool isInitialized_ = false;
};
