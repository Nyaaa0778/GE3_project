#include "Camera.h"
#include "D3DResourceLeakChecker.h"
#include "DirectXCommon.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "MathUtility.h"
#include "ModelManager.h"
#include "Object3d.h"
#include "Object3dRenderer.h"
#include "ShaderResourceViewManager.h"
#include "Sprite.h"
#include "SpriteRenderer.h"
#include "TextureManager.h"
#include "WinApp.h"

#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "SoundManager.h"

#include <Matrix4x4.h>
#include <Transform.h>
#include <Vector2.h>
#include <Vector3.h>
#include <Vector4.h>

#include <chrono>
#include <cstdint>
#include <dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#endif

#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <strsafe.h>

using namespace MathUtility;

using Microsoft::WRL::ComPtr;

// struct ChunkHeader {
//   char id[4];   // チャンクごとのID
//   int32_t size; // チャンクサイズ
// };
//
// struct RiffHeader {
//   ChunkHeader chunk; // RIFF
//   char type[4];      // WAVE
// };
//
// struct FormatChunk {
//   ChunkHeader chunk; // fmt
//   WAVEFORMATEX fmt;  // 波形フォーマット
// };

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

void Log(std::ostream &os, const std::string &message) {
  os << message << std::endl;
  OutputDebugStringA(message.c_str());
}

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

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

  D3DResourceLeakChecker leakCheck;

  CoInitializeEx(0, COINIT_MULTITHREADED);

  SetUnhandledExceptionFilter(ExportDump);

  /// =============================================
  ///
  /// Windowの初期化
  ///
  /// =============================================

  // WindowsAppのポインタ
  WinApp *winApp = nullptr;

  // WindowsAppの初期化
  winApp = new WinApp();
  winApp->Initialize();

  // 出力ウィンドウへの文字出力
  OutputDebugStringA("Hello,DirectX!\n");

  // ログのディレクトリを用意
  std::filesystem::create_directory("logs");

  /// =============================================
  ///
  /// DirectX12の初期化
  ///
  /// =============================================

  // ポインタ
  DirectXCommon *dxCommon = nullptr;

  // DirectXの初期化
  dxCommon = new DirectXCommon();
  dxCommon->Initialize(winApp);

  ShaderResourceViewManager *srvManager = nullptr;
  // SRVマネージャの初期化
  srvManager = new ShaderResourceViewManager();
  srvManager->Initialize(dxCommon);

  // ポインタ
  SpriteRenderer *spriteRenderer = nullptr;
  // スプライト共通部の初期化
  spriteRenderer = new SpriteRenderer();
  spriteRenderer->Initialize(dxCommon);

  // テクスチャマネージャの初期化
  TextureManager::GetInstance()->Initialize(dxCommon, srvManager);

  // ポインタ
  Sprite *sprite = nullptr;
  // スプライトの初期化
  sprite = new Sprite();
  sprite->Initialize(spriteRenderer, "resources/uvChecker.png");

  // ポインタ
  Object3dRenderer *object3dRenderer = nullptr;
  // object3dRendererの初期化
  object3dRenderer = new Object3dRenderer();
  object3dRenderer->Initialize(dxCommon);

  // 3Dモデルマネージャの初期化
  ModelManager::GetInstance()->Initialize(object3dRenderer->GetDxCommon());

  Camera *camera = new Camera();
  camera->SetRotate({0.0f, 0.0f, 0.0f});
  camera->SetTranslate({0.0f, 0.0f, -10.0f});
  object3dRenderer->SetDefaultCamera(camera);

  // ポインタ
  Object3d *object3d = nullptr;
  // object3dの初期化
  object3d = new Object3d();
  object3d->Initialize(object3dRenderer, "plane");

#ifdef USE_IMGUI

  ImGuiManager *imguiManager = nullptr;
  imguiManager = new ImGuiManager();
  imguiManager->Initialize(winApp, dxCommon, srvManager);

#endif

  // ParticleManager *particleManager = new ParticleManager();
  // particleManager->Initialize(dxCommon, srvManager);
  // particleManager->CreateParticleGroup("smoke",               // グループ名
  //                                      "resources/circle.png" // テクスチャ
  //);

  // particleManager->Emit("smoke", {0, 0, 0}, 2);

  ParticleEmitter *emitter = nullptr;

  ParticleManager::GetInstance()->Initialize(dxCommon, srvManager);

  ParticleManager::GetInstance()->CreateParticleGroup("smoke",
                                                      "resources/circle.png");

  // ★ Emitterを作る
  Transform emitterTransform{};
  emitterTransform.translation = {0.0f, 0.0f, 0.0f};

  emitter = new ParticleEmitter("smoke",           // グループ名
                                &emitterTransform, // 発生位置
                                0.1f,              // 発生間隔（秒）
                                2,                 // 1回に出す数
                                true               // 有効
  );

  /// =============================================
  ///
  /// 入力処理の初期化
  ///
  /// =============================================

  // 入力のポインタ
  Input *input = nullptr;

  // 入力の初期化
  input = new Input();
  input->Initialize(winApp);

  // --- SoundManager ---
  SoundManager *soundManager = new SoundManager();
  soundManager->Initialize();

  // 音声読み込み（パスは自分の resources に合わせて）
  soundManager->Load("resources/title.mp3");

  // object3d->SetModel("axis");

  // ウィンドウの×ボタンが押されるまでループ
  while (true) {

    if (winApp->ProcessMessage()) {
      // ゲームループを終了
      break;
    }

    // 入力更新
    input->Update();

#ifdef USE_IMGUI

    imguiManager->Begin();

    // ゲームの処理

    ImGui::Begin("Window");

    // ─────────────────────
    // ライト
    // ─────────────────────

    ImGui::SeparatorText("Directional Light");

    Vector4 color = object3d->GetLightColor();
    if (ImGui::ColorEdit3("Light Color", &color.x)) {
      object3d->SetLightColor({color.x, color.y, color.z, 1.0f});
    }

    Vector3 dir = object3d->GetLightDirection();
    if (ImGui::SliderFloat3("Direction", &dir.x, -1.0f, 1.0f)) {
      object3d->SetLightDirection(dir); // ← そのまま渡す
    }

    float intensity = object3d->GetLightIntensity();
    if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 5.0f)) {
      object3d->SetLightIntensity(intensity);
    }

    // ─────────────────────
    // Obj
    // ─────────────────────

    ImGui::SeparatorText("Object");

    {
      Vector3 pos = object3d->GetPosition();
      if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
        object3d->SetPosition(pos);
      }
    }

    {
      Vector3 scale = object3d->GetScale();
      if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, -10.0f, 10.0f)) {
        object3d->SetScale(scale);
      }
    }

    {
      Vector3 rot = object3d->GetRotate(); // ← 正しい！
      if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f, -6.28f, 6.28f)) {
        object3d->SetRotation(rot);
      }
    }

    {
      Vector4 color = object3d->GetColor();
      float col[4] = {color.x, color.y, color.z, color.w};

      // ImGui カラーピッカー
      if (ImGui::ColorEdit4("Color", col)) {

        // float[4] → Vector4 に戻す
        Vector4 newColor(col[0], col[1], col[2], col[3]);

        // Object3d 経由で Model に反映
        object3d->SetColor(newColor);
      }
    }

    {
      int mode = static_cast<int>(object3d->GetBlendMode());
      if (ImGui::Combo("BlendMode", &mode,
                       "None\0Normal\0Add\0Subtract\0Multiply\0Screen")) {
        object3d->SetBlendMode(static_cast<Object3dRenderer::BlendMode>(mode));
      }
    }

    // ─────────────────────
    // カメラ
    // ─────────────────────

    ImGui::SeparatorText("Camera");

    // 位置
    {
      Vector3 pos = camera->GetTranslate();
      if (ImGui::DragFloat3("Camera Position", &pos.x, 0.1f)) {
        camera->SetTranslate(pos);
      }
    }

    // 回転（ラジアン or 度はお好みで）
    {
      Vector3 rot = camera->GetRotate();
      if (ImGui::DragFloat3(" Camera Rotation", &rot.x, 0.01f)) {
        camera->SetRotate(rot);
      }
    }

    ImGui::End();

    imguiManager->End();

#endif

    /////
    ///// 更新処理 ↓
    /////

    sprite->SetPosition(Vector2{640.0f, 360.0f});
    sprite->SetAnchorPoint(Vector2{0.5f, 0.5f});
    sprite->SetFlipY(false);

    camera->Update();

    sprite->Update();
    object3d->Update();

    emitter->Update();

    ParticleManager::GetInstance()->Update(camera->GetViewMatrix(),
                                           camera->GetProjectionMatrix());

    if (input->TriggerKey(DIK_SPACE)) {
      soundManager->Play(soundManager);
    }

    /////
    ///// 更新処理 ↑
    /////

    /////
    ///// 描画処理 ↓
    /////

    srvManager->BeginDraw();
    dxCommon->BeginDraw();

    // sprite->Draw();
    // object3d->Draw();

    ParticleManager::GetInstance()->Draw();

#ifdef USE_IMGUI

    imguiManager->Draw();

#endif

    dxCommon->EndDraw();

    /////
    ///// 描画処理 ↑
    /////

    TextureManager::GetInstance()->ReleaseIntermediateResources();
  }

  soundManager->Finalize();
  delete soundManager;

#ifdef USE_IMGUI

  imguiManager->Finalize();

#endif

  //// 解放処理
  // CloseWindow(winApp->GetHwnd());

  // inputを解放
  delete input;

  // ParticleEmitterを解放
  delete emitter;

  ParticleManager::GetInstance()->Finalize();

  // object3dを解放
  delete object3d;

  // object3dRendererを解放
  delete object3dRenderer;

  // spriteを解放
  delete sprite;

  // spriteCommonを解放
  delete spriteRenderer;

  TextureManager::GetInstance()->Finalize();
  ModelManager::GetInstance()->Finalize();

  // SrvManager
  delete srvManager;

  // DirectXを解放
  delete dxCommon;

  // WinodwsAPIの終了処理
  winApp->Finalize();

  // WIndowsAPIを解放
  delete winApp;

  leakCheck.~D3DResourceLeakChecker();

  /*CoUninitialize();*/

  return 0;
}