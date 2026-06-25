#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include <string>

class SocketServer {
public:
    SocketServer();
    ~SocketServer();

    // サーバーを開始（デフォルトポート 12345）
    bool Start(int port = 12345);
    // サーバーを停止
    void Stop();

    bool IsConnected() const { return isConnected_; }
    int GetPort() const { return port_; }

    // Blenderから新しい同期フレームを受信したか確認し、あれば取得する
    bool GetTargetFrame(int& outFrame);
    
    // Blenderからテイクオーバー（再開）コマンドを受信したか確認する
    bool CheckTakeover();

    // Blenderからリプレイログロード命令を受信したか確認し、あれば取得する
    bool GetLoadReplayPath(std::string& outPath);

private:
    void RunServer();
    void HandleClient(SOCKET clientSocket);

    int port_;
    std::atomic<bool> isRunning_;
    std::atomic<bool> isConnected_;
    std::thread serverThread_;

    std::atomic<int> targetFrame_;
    std::atomic<bool> hasTargetFrame_;
    std::atomic<bool> hasTakeover_;

    std::string targetReplayPath_;
    std::atomic<bool> hasTargetReplayPath_;

    SOCKET listenSocket_;
};
