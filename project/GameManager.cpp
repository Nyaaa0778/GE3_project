#include "GameManager.h"

#include "Camera.h"
#include "D3DResourceLeakChecker.h"
#include "DirectXCommon.h"
#include "Input.h"
#include "ModelManager.h"
#include "Object3dRenderer.h"
#include "ShaderResourceViewManager.h"
#include "SoundManager.h"
#include "SpriteRenderer.h"
#include "TextureManager.h"
#include "WinApp.h"

#include <dbghelp.h>
#include <strsafe.h>
#pragma comment(lib, "Dbghelp.lib")
#include <filesystem>
#include <fstream>

// 現在時刻を取得
std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
// ログファイルの名前を秒にする
std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
    nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
// 日本時間(PCの設定時間)に変換
std::chrono::zoned_time localTime{std::chrono::current_zone(), nowSeconds};
// formatを使って年月日_時分秒の文字列に変換
std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
// 時刻を使ってファイル名を決定
std::string logFilePath = std::string("logs/") + dateString + ".log";
// ファイルを作って書き込み準備
std::ofstream logStream(logFilePath);

/// <summary>
/// デバッグ用のダンプを出力
/// </summary>
/// <param name="exception"></param>
/// <returns></returns>
static LONG WINAPI ExportDump(EXCEPTION_POINTERS *exception) {
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
  MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{0};
  minidumpInformation.ThreadId = threadId;
  minidumpInformation.ExceptionPointers = exception;
  minidumpInformation.ClientPointers = TRUE;
  // Dumpを出力
  MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle,
                    MiniDumpNormal, &minidumpInformation, nullptr, nullptr);

  return EXCEPTION_EXECUTE_HANDLER;
}

void GameManager::Initialize() {
  // WinAppの初期化
  winApp_ = new WinApp();
  winApp_->Initialize();

  // DirectXCommonの初期化
  dxCommon_ = new DirectXCommon();
  dxCommon_->Initialize(winApp_);

  // Inputの初期化
  input_ = new Input();
  input_->Initialize(winApp_);

  // srvManagerの初期化
  srvManager_ = new ShaderResourceViewManager();
  srvManager_->Initialize(dxCommon_);

  // SpriteRendererの初期化
  spriteRenderer_ = new SpriteRenderer();
  spriteRenderer_->Initialize(dxCommon_);

  // Object3dRendererの初期化
  object3dRenderer_ = new Object3dRenderer();
  object3dRenderer_->Initialize(dxCommon_);

  // SoundManagerの初期化
  soundManager_ = new SoundManager();
  soundManager_->Initialize();

  std::filesystem::create_directory("logs");

  SetUnhandledExceptionFilter(ExportDump);

  D3DResourceLeakChecker leakCheck;

  TextureManager::GetInstance()->Initialize(dxCommon_, srvManager_);
  ModelManager::GetInstance()->Initialize(object3dRenderer_->GetDxCommon());
}
