#include "GameFramework.h"

#include "Camera.h"
#include "DirectXCommon.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include "Object3dRenderer.h"
#include "ShaderResourceViewManager.h"
#include "AudioManager.h"
#include "SpriteRenderer.h"
#include "TextureManager.h"
#include "ParticleManager.h"
#include "WinApp.h"
#include "LightManager.h"
#include "SkyboxRenderer.h"
#include "PrimitiveRenderer.h"

#ifdef USE_IMGUI
#include "DebugManager.h"
#endif

#include <dbghelp.h>
#include <strsafe.h>
#pragma comment(lib, "Dbghelp.lib")
#include <chrono>
#include <filesystem>
#include <format>

/// <summary>
/// デバッグ用のダンプを出力
/// </summary>
/// <param name="exception"></param>
/// <returns></returns>
static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = {0};
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp",
		time.wYear, time.wMonth, time.wDay, time.wHour,
		time.wMinute);
	HANDLE dumpFileHandle =
		CreateFile(filePath, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();
	// 設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation {0};
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;
	// Dumpを出力
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle,
		MiniDumpNormal, &minidumpInformation, nullptr, nullptr);

	return EXCEPTION_EXECUTE_HANDLER;
}

GameFramework::GameFramework() = default;
GameFramework::~GameFramework() = default;

/// <summary>
/// 実行
/// </summary>
void GameFramework::Execute() {
	// ゲームの初期化
	Initialize();

	// ゲームループ
	while (true) {

#ifdef USE_IMGUI
		ImGuiManager::GetInstance()->Begin();
#endif

		// 毎フレーム更新
		Update();

#ifdef USE_IMGUI
		ImGuiManager::GetInstance()->End();
#endif

		// 終了リクエストが着たら抜ける
		if (IsEndRequest()) {
			break;
		}

		// 描画前処理
		BeginFrame();

		// 描画
		Draw();

		// 描画後処理
		EndFrame();
	}

	// ゲームの終了
	Finalize();
}

/// <summary>
/// 初期化
/// </summary>
void GameFramework::Initialize() {

	// ログシステムの初期化
	InitializeLogSystem();
	// 例外フィルタの設定
	SetUnhandledExceptionFilter(ExportDump);

	// WinApp の初期化
	WinApp::GetInstance()->Initialize(L"DirectXGame", 1280, 720);

	// DirectXCommon の初期化
	DirectXCommon::GetInstance()->Initialize(WinApp::GetInstance());

	// srvManage rの初期化
	ShaderResourceViewManager::GetInstance()->Initialize();

	// カメラの初期化
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({0.3f, 0.0f, 0.0f});
	camera_->SetTranslate({0.0f, 6.0f, -10.0f});
	camera_->CalculateMatrix();
	camera_->CreateConstantBuffer();

	// SpriteRenderer の初期化
	SpriteRenderer::GetInstance()->Initialize(DirectXCommon::GetInstance());

	// ModelCommon の初期化
	ModelCommon::GetInstance()->Initialize(DirectXCommon::GetInstance());

	// LightManager の初期化
	LightManager::GetInstance()->Initialize(DirectXCommon::GetInstance());

	// Object3dRenderer の初期化
	Object3dRenderer::GetInstance()->Initialize(DirectXCommon::GetInstance());
	// カメラをセット
	Object3dRenderer::GetInstance()->SetDefaultCamera(camera_.get());

	// SkyboxRenderer の初期化
	SkyboxRenderer::GetInstance()->Initialize(DirectXCommon::GetInstance());

	// PrimitiveRenderer の初期化
	PrimitiveRenderer::GetInstance()->Initialize(DirectXCommon::GetInstance());

	// AudioManager の初期化
	AudioManager::GetInstance()->Initialize();

	// TextureManafer の初期化
	TextureManager::GetInstance()->Initialize(
		DirectXCommon::GetInstance(), ShaderResourceViewManager::GetInstance());

	// ParticleManager の初期化
	ParticleManager::GetInstance()->Initialize(DirectXCommon::GetInstance(), ShaderResourceViewManager::GetInstance());

	// Input の初期化
	Input::GetInstance()->Initialize(WinApp::GetInstance());

#ifdef USE_IMGUI
	// ImGuiManager の初期化
	ImGuiManager::GetInstance()->Initialize(
		DirectXCommon::GetInstance(), ShaderResourceViewManager::GetInstance());

	// DebugManagerの初期化（TCPサーバー起動）
	DebugManager::GetInstance()->Initialize();
#endif
}

/// <summary>
/// 更新
/// </summary>
void GameFramework::Update() {
	if (WinApp::GetInstance()->ProcessMessage()) {
		endRequest_ = true;
	}

	if (Input::GetInstance()->TriggerKey(DIK_ESCAPE)) {
		endRequest_ = true;
	}

	// 入力の更新
	Input::GetInstance()->Update();

#ifdef USE_IMGUI
	// DebugManagerの更新（フレーム記録・コマンド処理・送信）
	DebugManager::GetInstance()->Update();
#endif
}

/// <summary>
/// 終了
/// </summary>
void GameFramework::Finalize() {
	if (logStream_.is_open()) {
		logStream_.close();
	}

#ifdef USE_IMGUI

	// DebugManagerの終了処理（ソケット・スレッドクリーンアップ）
	DebugManager::GetInstance()->Finalize();

	ImGuiManager::GetInstance()->Finalize();

#endif

	//// 解放処理
	// CloseWindow(winApp->GetHwnd());

	// inputを解放
	Input::GetInstance()->Finalize();

	// object3dRendererを解放
	Object3dRenderer::GetInstance()->Finalize();

	// PrimitiveRendererを解放
	PrimitiveRenderer::GetInstance()->Finalize();

	// modelCommonを解放
	ModelCommon::GetInstance()->Finalize();

	// ModelManagerを解放
	ModelManager::GetInstance()->Finalize();

	// spriteCommonを解放
	SpriteRenderer::GetInstance()->Finalize();

	// AudioManagerを解放
	AudioManager::GetInstance()->Finalize();

	TextureManager::GetInstance()->Finalize();

	// SrvManager
	ShaderResourceViewManager::GetInstance()->Finalize();

	// DirectXを解放
	DirectXCommon::GetInstance()->Finalize();

	// WIndowsAPIを解放
	WinApp::GetInstance()->Finalize();
}

/// <summary>
/// ログシステムの初期化
/// </summary>
void GameFramework::InitializeLogSystem() {
	std::filesystem::create_directory("logs");

	// 現在時刻を取得
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	// ログファイルの名前を秒にする
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
		nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
	// 日本時間(PCの設定時間)に変換
	std::chrono::zoned_time localTime {std::chrono::current_zone(), nowSeconds};
	// formatを使って年月日_時分秒の文字列に変換
	std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
	// 時刻を使ってファイル名を決定
	std::string logFilePath = std::string("logs/") + dateString + ".log";

	logStream_.open(logFilePath);

	// 試しに書き込み
	if (logStream_.is_open()) {
		logStream_ << "Game Initialized." << std::endl;
	}
}

/// <summary>
/// 描画前処理
/// </summary>
void GameFramework::BeginFrame() {
	ShaderResourceViewManager::GetInstance()->BeginDraw();

	DirectXCommon::GetInstance()->BeginDraw();
}

/// <summary>
/// 描画後処理
/// </summary>
void GameFramework::EndFrame() {

#ifdef USE_IMGUI

	ImGuiManager::GetInstance()->Draw();

#endif

	DirectXCommon::GetInstance()->EndDraw();

	TextureManager::GetInstance()->ReleaseIntermediateResources();
}
