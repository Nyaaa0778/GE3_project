#include "DebugNetwork.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

DebugNetwork* DebugNetwork::GetInstance() {
	static DebugNetwork instance;
	return &instance;
}

void DebugNetwork::Initialize(int port) {
	if (isInitialized_) return;

	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != 0) {
		return;
	}

	sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_ == INVALID_SOCKET) {
		WSACleanup();
		return;
	}

	// 非ブロッキングモードに設定
	u_long mode = 1;
	ioctlsocket(sock_, FIONBIO, &mode);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		closesocket(sock_);
		sock_ = INVALID_SOCKET;
		WSACleanup();
		return;
	}

	isInitialized_ = true;
}

void DebugNetwork::Finalize() {
	if (!isInitialized_) return;

	if (sock_ != INVALID_SOCKET) {
		closesocket(sock_);
		sock_ = INVALID_SOCKET;
	}
	WSACleanup();
	isInitialized_ = false;
}

int DebugNetwork::UpdateReceive() {
	if (!isInitialized_) return -1;

	char buf[256];
	sockaddr_in from;
	int fromLen = sizeof(from);
	int len = recvfrom(sock_, buf, sizeof(buf) - 1, 0, (sockaddr*)&from, &fromLen);

	if (len > 0) {
		buf[len] = '\0';
		std::string msg(buf);
		// メッセージ形式は "FRAME:<フレーム番号>"
		if (msg.rfind("FRAME:", 0) == 0) {
			try {
				return std::stoi(msg.substr(6));
			} catch (...) {
				return -1;
			}
		}
	}
	return -1;
}
