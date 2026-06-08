#include "SocketServer.h"
#include <iostream>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

SocketServer::SocketServer()
    : port_(12345),
      isRunning_(false),
      isConnected_(false),
      targetFrame_(0),
      hasTargetFrame_(false),
      hasTakeover_(false),
      listenSocket_(INVALID_SOCKET) {}

SocketServer::~SocketServer() {
    Stop();
}

bool SocketServer::Start(int port) {
    if (isRunning_) return true;

    port_ = port;
    isRunning_ = true;
    hasTargetFrame_ = false;
    hasTakeover_ = false;

    // Winsock初期化
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        isRunning_ = false;
        return false;
    }

    // リッスン用ソケット作成
    listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket_ == INVALID_SOCKET) {
        WSACleanup();
        isRunning_ = false;
        return false;
    }

    // ソケット再利用オプションを設定 (即座の再起動用)
    char optval = 1;
    setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(static_cast<USHORT>(port_));

    result = bind(listenSocket_, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        closesocket(listenSocket_);
        listenSocket_ = INVALID_SOCKET;
        WSACleanup();
        isRunning_ = false;
        return false;
    }

    result = listen(listenSocket_, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        closesocket(listenSocket_);
        listenSocket_ = INVALID_SOCKET;
        WSACleanup();
        isRunning_ = false;
        return false;
    }

    // バックグラウンドスレッド起動
    serverThread_ = std::thread(&SocketServer::RunServer, this);
    return true;
}

void SocketServer::Stop() {
    isRunning_ = false;
    isConnected_ = false;

    if (listenSocket_ != INVALID_SOCKET) {
        closesocket(listenSocket_);
        listenSocket_ = INVALID_SOCKET;
    }

    if (serverThread_.joinable()) {
        serverThread_.join();
    }

    WSACleanup();
}

bool SocketServer::GetTargetFrame(int& outFrame) {
    if (hasTargetFrame_) {
        outFrame = targetFrame_.load();
        hasTargetFrame_ = false;
        return true;
    }
    return false;
}

bool SocketServer::CheckTakeover() {
    bool takeover = hasTakeover_.load();
    if (takeover) {
        hasTakeover_ = false;
    }
    return takeover;
}

void SocketServer::RunServer() {
    while (isRunning_) {
        // 接続待ち
        SOCKET clientSocket = accept(listenSocket_, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            // エラーかサーバー終了時
            if (!isRunning_) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        isConnected_ = true;
        HandleClient(clientSocket);
        isConnected_ = false;
    }
}

void SocketServer::HandleClient(SOCKET clientSocket) {
    char buffer[1024];
    std::string lineBuffer = "";

    // タイムアウト設定 (ノンブロッキング気味に切断をチェックするため)
    DWORD timeout = 1000; // 1秒
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));

    while (isRunning_ && isConnected_) {
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead < 0) {
            int err = WSAGetLastError();
            if (err == WSAETIMEDOUT) {
                // タイムアウトならループ継続
                continue;
            }
            // その他のエラーは切断とみなす
            break;
        }
        if (bytesRead == 0) {
            // クライアント正常切断
            break;
        }

        buffer[bytesRead] = '\0';
        lineBuffer += buffer;

        // 改行コード（\n）で分割してコマンド処理
        size_t newlinePos;
        while ((newlinePos = lineBuffer.find('\n')) != std::string::npos) {
            std::string msg = lineBuffer.substr(0, newlinePos);
            lineBuffer = lineBuffer.substr(newlinePos + 1);

            // \r の除去
            if (!msg.empty() && msg.back() == '\r') {
                msg.pop_back();
            }

            // コマンドの解析
            if (msg.rfind("FRAME ", 0) == 0) {
                try {
                    int frameVal = std::stoi(msg.substr(6));
                    targetFrame_ = frameVal;
                    hasTargetFrame_ = true;
                } catch (...) {
                    // 数値変換エラー無視
                }
            } else if (msg == "TAKEOVER") {
                hasTakeover_ = true;
            }
        }
    }

    closesocket(clientSocket);
}
