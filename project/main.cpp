#include "DirectXCommon.h"
#include "Input.h"
#include "WinApp.h"
#include"D3DResourceLeakChecker.h"
#include"SpriteCommon.h"
#include"Sprite.h"

// #include <Windows.h>
#include <chrono>
#include <cstdint>

// #include <d3d12.h>
// #pragma comment(lib, "d3d12.lib")
// #include <dxgi1_6.h>
// #pragma comment(lib, "dxgi.lib")
// #include <cassert>
#include <dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
// #include <dxcapi.h>
// #pragma comment(lib, "dxcompiler.lib")

#include "externals/DirectXTex/DirectXTex.h"
#pragma comment(lib, "DirectXTex.lib")
#include "externals/DirectXTex/d3dx12.h"
#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <strsafe.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>

#include <sstream>

// #define DIRECTINPUT_VERSION 0x0800
// #include <dinput.h>

// #pragma comment(lib, "dinput8.lib")
// #pragma comment(lib, "dxguid.lib")

#include "MathUtility.h"
#include "Sound.h"

#include <Matrix4x4.h>
#include <Vector2.h>
#include <Vector3.h>
#include <Vector4.h>
#include<Transform.h>
using namespace MathUtility;
// #include <wrl.h>

using Microsoft::WRL::ComPtr;

struct TransformMatrix {
  Matrix4x4 WVP;
  Matrix4x4 World;
};

struct VertexData {
  Vector4 position;
  Vector2 texcoord;
  Vector3 normal;
};

struct Material {
  Vector4 color;
  int32_t enableLighting;
  float padding[3];
  Matrix4x4 uvTransform;
};

struct MaterialData {
  std::string textureFilePath;
};

struct DirectionalLight {
  Vector4 color;     // ライトの色
  Vector3 direction; // ライトの向き
  float intensity;   // 輝度
};

struct ModelData {
  std::vector<VertexData> vertices;
  MaterialData material;
};

struct ChunkHeader {
  char id[4];   // チャンクごとのID
  int32_t size; // チャンクサイズ
};

struct RiffHeader {
  ChunkHeader chunk; // RIFF
  char type[4];      // WAVE
};

struct FormatChunk {
  ChunkHeader chunk; // fmt
  WAVEFORMATEX fmt;  // 波形フォーマット
};

///// <summary>
///// std::wstringのメッセージを出力ウィンドウに表示
///// </summary>
///// <param name="message">メッセージの内容</param>
// void Log(const std::wstring &message) { OutputDebugStringW(message.c_str());
// }

///// <summary>
///// std::stringのメッセージを出力ウィンドウに表示
///// </summary>
///// <param name="message">メッセージの内容</param>
// void Log(const std::string &message) { OutputDebugStringA(message.c_str()); }

///// <summary>
///// std::stringからstd::wstringに変換
///// </summary>
///// <param name="str"></param>
///// <returns></returns>
// std::wstring ConvertString(const std::string &str) {
//   if (str.empty()) {
//     return std::wstring();
//   }
//
//   auto sizeNeeded =
//       MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char
//       *>(&str[0]),
//                           static_cast<int>(str.size()), NULL, 0);
//   if (sizeNeeded == 0) {
//     return std::wstring();
//   }
//   std::wstring result(sizeNeeded, 0);
//   MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(&str[0]),
//                       static_cast<int>(str.size()), &result[0], sizeNeeded);
//   return result;
// }
//
///// <summary>
///// std::wstringからstd::stringに変換
///// </summary>
///// <param name="str"></param>
///// <returns></returns>
// std::string ConvertString(const std::wstring &str) {
//   if (str.empty()) {
//     return std::string();
//   }
//
//   auto sizeNeeded =
//       WideCharToMultiByte(CP_UTF8, 0, str.data(),
//       static_cast<int>(str.size()),
//                           NULL, 0, NULL, NULL);
//   if (sizeNeeded == 0) {
//     return std::string();
//   }
//   std::string result(sizeNeeded, 0);
//   WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
//                       result.data(), sizeNeeded, NULL, NULL);
//   return result;
// }

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

/// <summary>
/// 透視投影行列
/// </summary>
/// <param name="fovY"></param>
/// <param name="aspectRatio"></param>
/// <param name="nearClip"></param>
/// <param name="farClip"></param>
/// <returns></returns>
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio,
                                   float nearClip, float farClip) {
  Matrix4x4 result;
  result.m[0][0] = 1.0f / aspectRatio * 1.0f / std::tan(fovY / 2.0f);
  result.m[0][1] = 0.0f;
  result.m[0][2] = 0.0f;
  result.m[0][3] = 0.0f;

  result.m[1][0] = 0.0f;
  result.m[1][1] = 1.0f / std::tan(fovY / 2.0f);
  result.m[1][2] = 0.0f;
  result.m[1][3] = 0.0f;

  result.m[2][0] = 0.0f;
  result.m[2][1] = 0.0f;
  result.m[2][2] = farClip / (farClip - nearClip);
  result.m[2][3] = 1.0f;

  result.m[3][0] = 0.0f;
  result.m[3][1] = 0.0f;
  result.m[3][2] = -nearClip * farClip / (farClip - nearClip);
  result.m[3][3] = 0.0f;

  return result;
}

///// <summary>
///// 正射影行列(3次元版)
///// </summary>
///// <param name="left"></param>
///// <param name="top"></param>
///// <param name="right"></param>
///// <param name="bottom"></param>
///// <param name="nearClip"></param>
///// <param name="farClip"></param>
///// <returns></returns>
//Matrix4x4 MakeOrthographicMatrix(float left, float top, float right,
//                                 float bottom, float nearClip, float farClip) {
//  Matrix4x4 result;
//  result.m[0][0] = 2.0f / (right - left);
//  result.m[0][1] = 0.0f;
//  result.m[0][2] = 0.0f;
//  result.m[0][3] = 0.0f;
//
//  result.m[1][0] = 0.0f;
//  result.m[1][1] = 2.0f / (top - bottom);
//  result.m[1][2] = 0.0f;
//  result.m[1][3] = 0.0f;
//
//  result.m[2][0] = 0.0f;
//  result.m[2][1] = 0.0f;
//  result.m[2][2] = 1.0f / (farClip - nearClip);
//  result.m[2][3] = 0.0f;
//
//  result.m[3][0] = (left + right) / (left - right);
//  result.m[3][1] = (top + bottom) / (bottom - top);
//  result.m[3][2] = nearClip / (nearClip - farClip);
//  result.m[3][3] = 1.0f;
//
//  return result;
//}

///// <summary>
///// 奥行情報を記録するためのバッファGPUに作る
///// </summary>
///// <param name="device"></param>
///// <param name="width"></param>
///// <param name="height"></param>
///// <returns></returns>
// ComPtr<ID3D12Resource>
// CreateDepthStencilTextureResource(const ComPtr<ID3D12Device> &device,
//                                   int32_t width, int32_t height) {
//   // 生成するResourceの設定
//   D3D12_RESOURCE_DESC resourceDesc{};
//   resourceDesc.Width = width;        // Textureの幅
//   resourceDesc.Height = height;      // Textureの高さ
//   resourceDesc.MipLevels = 1;        // mipmapの数
//   resourceDesc.DepthOrArraySize = 1; // 奥行 or 配列Textureの配列数
//   resourceDesc.Format =
//       DXGI_FORMAT_D24_UNORM_S8_UINT; //
//       DepthStencilとして利用可能なフォーマット
//   resourceDesc.SampleDesc.Count = 1; // サンプリングカウント、1固定
//   resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 二次元
//   resourceDesc.Flags =
//       D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知
//
//   // 利用するHeapの設定
//   D3D12_HEAP_PROPERTIES heapProperties{};
//   heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る
//
//   // 深度値のクリア設定
//   D3D12_CLEAR_VALUE depthClearValue{};
//   depthClearValue.DepthStencil.Depth = 1.0f; // 最大値(一番遠い状態)
//   depthClearValue.Format =
//       DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット、Resourceと合わせる
//
//   // Resourceの生成
//   ComPtr<ID3D12Resource> resource = nullptr;
//   HRESULT hr = device->CreateCommittedResource(
//       &heapProperties,                  // Heapの設定
//       D3D12_HEAP_FLAG_NONE,             // Heaoの特殊な設定
//       &resourceDesc,                    // Resourceの設定
//       D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込む状態にする
//       &depthClearValue,                 // Clear最適地
//       IID_PPV_ARGS(&resource)           //
//       作成するResourceポインタへのポインタ
//   );
//   assert(SUCCEEDED(hr));
//
//   return resource;
// }

// D3D12_CPU_DESCRIPTOR_HANDLE
// GetCPUDescriptorHandle(const ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
//                        uint32_t descriptorSize, uint32_t index) {
//   D3D12_CPU_DESCRIPTOR_HANDLE handleCPU =
//       descriptorHeap->GetCPUDescriptorHandleForHeapStart();
//   handleCPU.ptr += (descriptorSize * index);
//
//   return handleCPU;
// }
//
// D3D12_GPU_DESCRIPTOR_HANDLE
// GetGPUDescriptorHandle(const ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
//                        uint32_t descriptorSize, uint32_t index) {
//   D3D12_GPU_DESCRIPTOR_HANDLE handleGPU =
//       descriptorHeap->GetGPUDescriptorHandleForHeapStart();
//   handleGPU.ptr += (descriptorSize * index);
//
//   return handleGPU;
// }

/// <summary>
/// mtlファイルを読む
/// </summary>
/// <param name="directoryPath"></param>
/// <param name="filename"></param>
/// <returns></returns>
MaterialData LoadMaterialTemplateFile(const std::string &directoryPath,
                                      const std::string &filename) {
  // 構築するMaterialData
  MaterialData materialData;
  // ファイルから読んだ1行を格納するもの
  std::string line;
  // ファイルを開く
  std::ifstream file(directoryPath + "/" + filename);
  assert(file.is_open());

  while (std::getline(file, line)) {
    std::string identifier;
    std::stringstream s(line);
    s >> identifier;

    if (identifier == "map_Kd") {
      std::string textureFilename;
      s >> textureFilename;
      // 凍結してファイルパスにする
      materialData.textureFilePath = directoryPath + "/" + textureFilename;
    }
  }

  return materialData;
}

/// <summary>
/// objファイルを読み込む
/// </summary>
/// <param name="directoryPath"></param>
/// <param name="filename"></param>
/// <returns></returns>
ModelData LoadObjFile(const std::string &directoryPath,
                      const std::string &filename) {
  ModelData modelData;            // 構築するModelData
  std::vector<Vector4> positions; // 位置
  std::vector<Vector3> normals;   // 法線
  std::vector<Vector2> texcoords; // テクスチャ座標
  std::string line;               // ファイルから読んだ1行を格納するもの

  std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
  assert(file.is_open()); // とりあえず開けなかったら止める

  while (std::getline(file, line)) {
    std::string identifier;
    std::istringstream s(line);
    s >> identifier; // 先頭の識別子を読む

    if (identifier == "v") {
      Vector4 position;
      s >> position.x >> position.y >> position.z;
      position.w = 1.0f;
      positions.push_back(position);
    } else if (identifier == "vt") {
      Vector2 texcoord;
      s >> texcoord.x >> texcoord.y;
      texcoords.push_back(texcoord);
    } else if (identifier == "vn") {
      Vector3 normal;
      s >> normal.x >> normal.y >> normal.z;
      normals.push_back(normal);
    } else if (identifier == "f") {
      VertexData triangle[3];

      // 面は三角形限定
      for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
        std::string vertexDefinition;
        s >> vertexDefinition;
        // 頂点の要素のIndexは 位置 / UV / 法線
        // で格納されているので、分解してIndexを取得
        std::istringstream v(vertexDefinition);

        uint32_t elementIndices[3];
        for (int32_t element = 0; element < 3; ++element) {
          std::string index;
          std::getline(v, index, '/');
          elementIndices[element] = std::stoi(index);
        }

        // 要素へのIndexから、実際の要素の値を取得して頂点を構築
        Vector4 position = positions[elementIndices[0] - 1];
        Vector2 texcoord = texcoords[elementIndices[1] - 1];
        Vector3 normal = normals[elementIndices[2] - 1];

        position.x *= -1.0f;
        texcoord.y = 1.0f - texcoord.y;
        normal.x *= -1.0f;

        triangle[faceVertex] = {position, texcoord, normal};
      }
      // 頂点を逆順で登録することで周り順を逆にする
      modelData.vertices.push_back(triangle[2]);
      modelData.vertices.push_back(triangle[1]);
      modelData.vertices.push_back(triangle[0]);
    } else if (identifier == "mtllib") {
      // materialTemolateLibraryファイルの名前を取得
      std::string materialFilename;
      s >> materialFilename;
      // 基本的にobjファイルと同一階層にmtlは存在させるのでディレクトリ名とファイル名を渡す
      modelData.material =
          LoadMaterialTemplateFile(directoryPath, materialFilename);
    }
  }

  return modelData;
}

///// <summary>
///// トリガー処理
///// </summary>
///// <param name="keyNumber"></param>
///// <param name="keys"></param>
///// <param name="preKeys"></param>
///// <returns></returns>
// bool isTrigger(uint8_t keyNumber, const BYTE *keys, const BYTE *preKeys) {
//   return ((keys[keyNumber] & 0x80) && !(preKeys[keyNumber] & 0x80));
// }

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

  //ポインタ
  SpriteCommon *spriteCommon = nullptr;
  //スプライト共通部の初期化
  spriteCommon = new SpriteCommon();
  spriteCommon->Initialize(dxCommon);

  //ポインタ
  Sprite *sprite = nullptr;
  //スプライトの初期化
  sprite = new Sprite();
  sprite->Initialize(spriteCommon);





  /// =============================================
  ///
  /// RootSignatureの初期化
  ///
  /// =============================================

  //// RootSignature作成
  // D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
  // descriptionRootSignature.Flags =
  //     D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  //// RootParamenterを作成、複数設定できるから配列(今回は結果1つなので長さ1の配列)
  // D3D12_ROOT_PARAMETER rootParameters[4] = {};
  // rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使用 
  // rootParameters[0].ShaderVisibility =
  //     D3D12_SHADER_VISIBILITY_PIXEL;               // PixelShaderで使う
  // rootParameters[0].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド
  // rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使用 
  // rootParameters[1].ShaderVisibility =
  //     D3D12_SHADER_VISIBILITY_VERTEX;              // PixelShaderで使う
  // rootParameters[1].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド

  // D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
  // descriptorRange[0].BaseShaderRegister = 0;                      // 0から
  // descriptorRange[0].NumDescriptors = 1;                          // 数は1つ
  // descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う 
  // descriptorRange[0].OffsetInDescriptorsFromTableStart =
  //     D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

  // rootParameters[2].ParameterType =
  //     D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
  // rootParameters[2].ShaderVisibility =
  //     D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderを使う
  // rootParameters[2].DescriptorTable.pDescriptorRanges =
  //     descriptorRange; // Tableの中身に配列を指定
  // rootParameters[2].DescriptorTable.NumDescriptorRanges =
  //     _countof(descriptorRange); // Tableで利用する数

  // rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う 
  // rootParameters[3].ShaderVisibility =
  //     D3D12_SHADER_VISIBILITY_PIXEL;               // PixelShaderを使う
  // rootParameters[3].Descriptor.ShaderRegister = 1; // レジスタ番号1を使う

  // descriptionRootSignature.pParameters =
  //     rootParameters; // ルートパラメータ配列へのポインタ
  // descriptionRootSignature.NumParameters =
  //     _countof(rootParameters); // 配列の長さ

  // D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
  // staticSamplers[0].Filter =
  //     D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
  // staticSamplers[0].AddressU =
  //     D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0~1の範囲外をリピート
  // staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  // staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  // staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; //   比較しない 
  // staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // たくさんのMipmapを使う
  // staticSamplers[0].ShaderRegister = 0;         //レジスタ番号0を使う 
  // staticSamplers[0].ShaderVisibility =
  // D3D12_SHADER_VISIBILITY_PIXEL; descriptionRootSignature.pStaticSamplers =
  // staticSamplers; descriptionRootSignature.NumStaticSamplers =
  // _countof(staticSamplers);

  //// シリアライズしてバイナリする
  // ID3DBlob *signatureBlob = nullptr;
  // ID3DBlob *errorBlob = nullptr;
  // HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
  //                                  D3D_ROOT_SIGNATURE_VERSION_1,
  //                                  &signatureBlob, &errorBlob);
  // if (FAILED(hr)) {
  //   /*Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));*/
  //   assert(false);
  // }
  //// バイナリをもとに生成
  // ComPtr<ID3D12RootSignature> rootSignature = nullptr;
  // hr = dxCommon->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
  //                                  signatureBlob->GetBufferSize(),
  //                                  IID_PPV_ARGS(&rootSignature));
  // assert(SUCCEEDED(hr));

  ///// =============================================
  /////
  ///// パイプラインステートの構成の準備
  /////
  ///// =============================================

  //// InputLayout
  // D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
  // inputElementDescs[0].SemanticName = "POSITION";
  // inputElementDescs[0].SemanticIndex = 0;
  // inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  // inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
  // inputElementDescs[1].SemanticName = "TEXCOORD";
  // inputElementDescs[1].SemanticIndex = 0;
  // inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
  // inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
  // inputElementDescs[2].SemanticName = "NORMAL";
  // inputElementDescs[2].SemanticIndex = 0;
  // inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  // inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
  // D3D12_INPUT_LAYOUT_DESC inputLayOutDesc{};
  // inputLayOutDesc.pInputElementDescs = inputElementDescs;
  // inputLayOutDesc.NumElements = _countof(inputElementDescs);

  //// BlendStateの設定
  // D3D12_BLEND_DESC blendDesc{};
  //// すべての色要素を書き込む
  // blendDesc.RenderTarget[0].RenderTargetWriteMask =
  //     D3D12_COLOR_WRITE_ENABLE_ALL;

  //// RasterizerStateの設定
  // D3D12_RASTERIZER_DESC rasterizerDesc{};
  //// 裏面(時計回り)を表示しない
  // rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
  //// 三角形の中を塗りつぶす
  // rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

  //// Shaderをコンパイル
  // ComPtr<IDxcBlob> vertexShaderBlob =
  //     dxCommon->CompileShader(L"resources/shaders/Object3D.VS.hlsl", L"vs_6_0");
  // assert(vertexShaderBlob != nullptr);

  // ComPtr<IDxcBlob> pixelShaderBlob = dxCommon->CompileShader(
  //     L"resources/shaders/Object3D.PS.hlsl", L"ps_6_0");
  // assert(pixelShaderBlob != nullptr);

  ///// =============================================
  /////
  ///// 深度情報
  /////
  ///// =============================================

  ////// DepthStencilTextureをウィンドウのサイズで作成
  ////ComPtr<ID3D12Resource> depthStencilResource =
  ////    CreateDepthStencilTextureResource(device, WinApp::kClientWidth,
  ////                                      WinApp::kClientHeight);

  //////// DSV用のヒープディスクリプタの数は1、ShaderVisibleはfalse
  //////ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap =
  //////    CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1,
  ///false);

  ////// DSVの設定
  ////D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
  ////dsvDesc.Format =
  ////    DXGI_FORMAT_D24_UNORM_S8_UINT; // FOrmat、基本的にはResourceに合わせる
  ////dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2dTexture
  ////// DSVHeapの先頭にDSVをつくる
  ////D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
  ////    GetCPUDescriptorHandle(dsvDescriptorHeap, descriptorSizeDSV, 0);

  ////device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc,
  ////                               dsvHandle);

  ////// DepthStencilStateの設定
  ////D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
  ////// Depthの機能を有効化
  ////depthStencilDesc.DepthEnable = true;
  ////// 書き込む
  ////depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  ////// 比較関数はLessEqual、近ければ描画される
  ////depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

  /// =============================================
  ///
  /// PSOを生成
  ///
  /// =============================================

  // D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
  // graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();
  // graphicsPipelineStateDesc.InputLayout = inputLayOutDesc;
  // graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
  //                                 vertexShaderBlob->GetBufferSize()};
  // graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
  //                                 pixelShaderBlob->GetBufferSize()};
  // graphicsPipelineStateDesc.BlendState = blendDesc;
  // graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

  //// DepthStencilの設定
  // graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

  //// 書き込むRTVの情報
  // graphicsPipelineStateDesc.NumRenderTargets = 1;
  // graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  //// 利用するトポロジ(形状)のタイプ、三角形
  // graphicsPipelineStateDesc.PrimitiveTopologyType =
  //     D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  //// どのように画面に色を打ち込むのか設定
  // graphicsPipelineStateDesc.SampleDesc.Count = 1;
  // graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
  //// 実際に生成
  // ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
  // hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(
  //     &graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
  // assert(SUCCEEDED(hr));

  ///// =============================================
  /////
  ///// モデルを読み込む
  /////
  ///// =============================================

  //// モデルの読み込み
  // ModelData modelData = LoadObjFile("resources", "plane.obj");
  //// 頂点リソースを作る
  // ComPtr<ID3D12Resource> vertexResourceModel = CreateBufferResource(
  //     device, sizeof(VertexData) * modelData.vertices.size());
  //// 頂点バッファビューを作成
  // D3D12_VERTEX_BUFFER_VIEW vertexBufferViewModel{};
  //// リソースの先頭のアドレスから使う
  // vertexBufferViewModel.BufferLocation =
  //     vertexResourceModel->GetGPUVirtualAddress();
  //// 使用するリソースのサイズは頂点のサイズ
  // vertexBufferViewModel.SizeInBytes =
  //     UINT(sizeof(VertexData) * modelData.vertices.size());
  //// 1頂点当たりのサイズ
  // vertexBufferViewModel.StrideInBytes = sizeof(VertexData);

  //// 頂点リソースにデータを書き込む
  // VertexData *vertexDataModel = nullptr;
  //// 書き込むためのアドレスを取得
  // vertexResourceModel->Map(0, nullptr,
  //                          reinterpret_cast<void **>(&vertexDataModel));
  //// 頂点データをリソースにコピー
  // std::memcpy(vertexDataModel, modelData.vertices.data(),
  //             sizeof(VertexData) * modelData.vertices.size());

  // vertexDataModel->normal = {0.0f, 0.0f, -1.0f};

  ///// =============================================
  /////
  ///// モデルのTransform
  /////
  ///// =============================================

  // ComPtr<ID3D12Resource> transformMatrixResourceModel =
  //     CreateBufferResource(device, sizeof(TransformMatrix));
  // TransformMatrix *transformMatrixDataModel = nullptr;
  // transformMatrixResourceModel->Map(
  //     0, nullptr, reinterpret_cast<void **>(&transformMatrixDataModel));
  //// 初期値は単位行列でもOK
  // transformMatrixDataModel->World = MakeIdentityMatrix();
  // transformMatrixDataModel->WVP = MakeIdentityMatrix();

  ///// =============================================
  /////
  ///// 球のリソースの作成と初期化
  /////
  ///// =============================================

  // const int kSubdivision = 16;

  //// 経度分割1つ分の角度φ
  // const float kLonEvery =
  //     static_cast<float>(M_PI) * 2.0f / static_cast<float>(kSubdivision);
  //// 緯度分割1つ分の角度θ
  // const float kLatEvery =
  //     static_cast<float>(M_PI) / static_cast<float>(kSubdivision);

  //// 球のリソース作成
  // ComPtr<ID3D12Resource> vertexResource = CreateBufferResource(
  //     device, sizeof(VertexData) * kSubdivision * kSubdivision * 6);

  //// 頂点バッファビューを作成する
  // D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
  //// リソースの先頭のアドレスから使う
  // vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
  //// 使用するリソースのサイズは頂点3つ分のサイズ
  // vertexBufferView.SizeInBytes =
  //     sizeof(VertexData) * kSubdivision * kSubdivision * 6;
  //// 1頂点あたりのサイズ
  // vertexBufferView.StrideInBytes = sizeof(VertexData);
  // VertexData *vertexData = nullptr;
  // vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));

  //// 緯度の方向に分割
  // for (int latIndex = 0; latIndex < kSubdivision; ++latIndex) {
  //   // このセルの上下緯度と V
  //   float lat0 = -static_cast<float>(M_PI) / 2.0f + kLatEvery * latIndex; //
  //   上 float lat1 = lat0 + kLatEvery; // 下 float v0 = 1.0f - float(latIndex)
  //   / kSubdivision; float v1 = 1.0f - float(latIndex + 1) / kSubdivision;

  //  for (int lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
  //    uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;

  //    // このセルの左右経度と U
  //    float lon0 = kLonEvery * lonIndex; // 左
  //    float lon1 = lon0 + kLonEvery;     // 右
  //    float u0 = float(lonIndex) / kSubdivision;
  //    float u1 = float(lonIndex + 1) / kSubdivision;

  //    // ── 三角形１ （左上→左下→右上）
  //    vertexData[start] = {{std::cos(lat0) * std::cos(lon0), std::sin(lat0),
  //                          std::cos(lat0) * std::sin(lon0), 1.0f},
  //                         {u0, v0}};
  //    vertexData[start + 1] = {{std::cos(lat1) * std::cos(lon0),
  //    std::sin(lat1),
  //                              std::cos(lat1) * std::sin(lon0), 1.0f},
  //                             {u0, v1}};
  //    vertexData[start + 2] = {{std::cos(lat0) * std::cos(lon1),
  //    std::sin(lat0),
  //                              std::cos(lat0) * std::sin(lon1), 1.0f},
  //                             {u1, v0}};

  //    // ── 三角形２ （右上→左下→右下）
  //    vertexData[start + 3] = {{std::cos(lat0) * std::cos(lon1),
  //    std::sin(lat0),
  //                              std::cos(lat0) * std::sin(lon1), 1.0f},
  //                             {u1, v0}};
  //    vertexData[start + 4] = {{std::cos(lat1) * std::cos(lon0),
  //    std::sin(lat1),
  //                              std::cos(lat1) * std::sin(lon0), 1.0f},
  //                             {u0, v1}};
  //    vertexData[start + 5] = {{std::cos(lat1) * std::cos(lon1),
  //    std::sin(lat1),
  //                              std::cos(lat1) * std::sin(lon1), 1.0f},
  //                             {u1, v1}};

  //    for (int i = 0; i < 6; ++i) {
  //      // 位置ベクトルの xyz をそのまま normal にセット
  //      vertexData[start + i].normal.x = vertexData[start + i].position.x;
  //      vertexData[start + i].normal.y = vertexData[start + i].position.y;
  //      vertexData[start + i].normal.z = vertexData[start + i].position.z;
  //    }
  //  }
  //}

  ///// =============================================
  /////
  ///// 三角形のリソースの作成と初期化
  /////
  ///// =============================================

  ////// 三角形のリソース作成
  //// ComPtr<ID3D12Resource> vertexResource =
  ////     CreateBufferResource(device, sizeof(VertexData) * 6);

  ////// 頂点バッファビューを作成する
  //// D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
  ////// リソースの先頭のアドレスから使う
  //// vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
  ////// 使用するリソースのサイズは頂点3つ分のサイズ
  //// vertexBufferView.SizeInBytes = sizeof(VertexData) * 6;
  ////// 1頂点あたりのサイズ
  //// vertexBufferView.StrideInBytes = sizeof(VertexData);

  ////// 頂点リソースにデータを書き込む
  //// VertexData *vertexData = nullptr;
  ////// 書き込むためのアドレスを取得
  //// vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));
  ////// 左下
  //// vertexData[0] = {{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}};
  ////// 上
  //// vertexData[1] = {{0.0f, 0.5f, 0.0f, 1.0f}, {0.5f, 0.0f}};
  ////// 右下
  //// vertexData[2] = {{0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}};

  ////// 左下
  //// vertexData[3] = {{-0.5f, -0.5f, 0.5f, 1.0f}, {0.0f, 1.0f}};
  ////// 上
  //// vertexData[4] = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.5f, 0.0f}};
  ////// 右下
  //// vertexData[5] = {{0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}};

  ///// =============================================
  /////
  ///// 球のマテリアルのリソースの作成と初期化
  /////
  ///// =============================================

  //// 三角形の色の変更
  // static float materialColor[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // RGBA

  //// マテリアル用のリソースを作成
  // ComPtr<ID3D12Resource> materialResource =
  //     CreateBufferResource(device, sizeof(Material));
  //// マテリアルにデータを書き込む
  // Material *materialData = nullptr;
  //// 書き込むためのアドレスを取得
  // materialResource->Map(0, nullptr, reinterpret_cast<void
  // **>(&materialData));
  //// 今回は白
  // materialData->color = Vector4{materialColor[0], materialColor[1],
  //                               materialColor[2], materialColor[3]};
  // materialData->enableLighting = true;
  // materialData->uvTransform = MakeIdentityMatrix();

  ///// =============================================
  /////
  ///// TransformationMatrix用のリソースの作成
  /////
  ///// =============================================

  //// WVP用のリソースを作成、Matrix4x4 1つ分のサイズ
  // ComPtr<ID3D12Resource> transformMatrixResource =
  //     CreateBufferResource(device, sizeof(TransformMatrix));
  //// データを書き込む
  // TransformMatrix *transformMatrixData = nullptr;
  //// 書き込むためのアドレスを取得
  // transformMatrixResource->Map(0, nullptr,
  //                              reinterpret_cast<void
  //                              **>(&transformMatrixData));
  //// 単位行列を書き込んでおく
  // transformMatrixData->WVP = MakeIdentityMatrix();
  // transformMatrixData->World = MakeIdentityMatrix();

  ///// =============================================
  /////
  ///// ライト用のリソースの作成
  /////
  ///// =============================================

  //// ライト用のリソース作成
  // ComPtr<ID3D12Resource> directionalLightResource =
  //     CreateBufferResource(device, sizeof(DirectionalLight));

  //// データを書き込む
  // DirectionalLight *directionalLightData = nullptr;
  //// 書き込むためのアドレスを取得
  // directionalLightResource->Map(
  //     0, nullptr, reinterpret_cast<void **>(&directionalLightData));

  // directionalLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
  // directionalLightData->direction = {0.0f, -1.0f, 0.0f};
  // directionalLightData->intensity = 1.0f;

  ///// =============================================
  /////
  ///// UI(2D)のリソースの作成と初期化
  /////
  ///// =============================================

  //// Sprite用の頂点リソースを作る
  // ComPtr<ID3D12Resource> vertexResourceSprite =
  //     CreateBufferResource(device, sizeof(VertexData) * 4);

  //// 頂点バッファビューを作成する
  // D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
  //// リソースの先頭のアドレスから使う
  // vertexBufferViewSprite.BufferLocation =
  //     vertexResourceSprite->GetGPUVirtualAddress();
  //// 使用するリソースのサイズは頂点6つ分のサイズ
  // vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 4;
  //// 1頂点当たりのサイズ
  // vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

  // VertexData *vertexDataSprite = nullptr;
  // vertexResourceSprite->Map(0, nullptr,
  //                           reinterpret_cast<void **>(&vertexDataSprite));
  //// 1枚目の三角形
  //// 左下
  // vertexDataSprite[0] = {{0.0f, 360.0f, 0.0f, 1.0f}, {0.0f, 1.0f}};
  //// 左上
  // vertexDataSprite[1] = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}};
  //// 右下
  // vertexDataSprite[2] = {{640.0f, 360.0f, 0.0f, 1.0f}, {1.0f, 1.0}};
  //// 右上
  // vertexDataSprite[3] = {{640.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}};

  //// Sprite用の頂点インデックスを作る
  // ComPtr<ID3D12Resource> indexResourceSprite =
  //     CreateBufferResource(device, sizeof(uint32_t) * 6);

  // D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
  //// リソースの先頭のアドレスから使う
  // indexBufferViewSprite.BufferLocation =
  //     indexResourceSprite->GetGPUVirtualAddress();
  //// 使用するリソースサイズはインデックスの6つ分のサイズ
  // indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
  //// インデックスはuint32_t
  // indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

  //// インデックスリソースにデータを書き込む
  // uint32_t *indexDataSprite = nullptr;
  // indexResourceSprite->Map(0, nullptr,
  //                          reinterpret_cast<void **>(&indexDataSprite));
  // indexDataSprite[0] = 0;
  // indexDataSprite[1] = 1;
  // indexDataSprite[2] = 2;
  // indexDataSprite[3] = 1;
  // indexDataSprite[4] = 3;
  // indexDataSprite[5] = 2;

  ///// =============================================
  /////
  ///// UI(2D)のマテリアルのリソースの作成と初期化
  /////
  ///// =============================================

  //// 三角形の色の変更
  // static float materialColorSprite[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // RGBA

  //// マテリアル用のリソースを作成
  // ComPtr<ID3D12Resource> materialResourceSprite =
  //     CreateBufferResource(device, sizeof(Material));
  //// マテリアルにデータを書き込む
  // Material *materialDataSprite = nullptr;
  //// 書き込むためのアドレスを取得
  // materialResourceSprite->Map(0, nullptr,
  //                             reinterpret_cast<void
  //                             **>(&materialDataSprite));
  //// 今回は白
  // materialDataSprite->color =
  //     Vector4{materialColorSprite[0], materialColorSprite[1],
  //             materialColorSprite[2], materialColorSprite[3]};
  // materialDataSprite->enableLighting = false;
  // materialDataSprite->uvTransform = MakeIdentityMatrix();

  ///// =============================================
  /////
  ///// UI(2D)用のTransformationMatrix用のリソースの作成
  /////
  ///// =============================================

  //// Matrix4x4 1つ分のサイズを用意
  // ComPtr<ID3D12Resource> transformationMatrixResourceSprite =
  //     CreateBufferResource(device, sizeof(TransformMatrix));
  //// データを書き込む
  // TransformMatrix *transformationMatrixDataSprite = nullptr;
  //// 書き込むためのアドレスを取得
  // transformationMatrixResourceSprite->Map(
  //     0, nullptr, reinterpret_cast<void
  //     **>(&transformationMatrixDataSprite));
  //// 単位行列に書き込んでおく
  // transformationMatrixDataSprite->WVP = MakeIdentityMatrix();
  // transformationMatrixDataSprite->World = MakeIdentityMatrix();

  ///// =============================================
  /////
  ///// Texture
  /////
  ///// =============================================

  //// テクスチャ数
  // const int kNumTextures = 2;

  //// 読み込むファイル名をまとめた配列
  // const char *textureFiles[kNumTextures] = {"resources/uvChecker.png",
  //                                           "resources/monsterBall.png"};

  ////
  ///ミップイメージ、メタデータ、GPUリソース、アップロード用インターメディエイトリソースをまとめて持つ配列
  // DirectX::ScratchImage mipImages[kNumTextures];
  // DirectX::TexMetadata metadata[kNumTextures];
  // ComPtr<ID3D12Resource> textureResources[kNumTextures] = {nullptr, nullptr};
  // ComPtr<ID3D12Resource> intermediateResources[kNumTextures] = {nullptr,
  //                                                               nullptr};

  //// ファイルを読み込んで ScratchImage に
  // mipImages[0] = LoadTexture(textureFiles[0]);
  //// メタデータ取得
  // metadata[0] = mipImages[0].GetMetadata();
  //// GPU 上のテクスチャリソースを作成
  // textureResources[0] = CreateTextureResource(device, metadata[0]);
  //// サブリソースをアップロードしてインターメディエイトバッファを取得
  // intermediateResources[0] =
  //     UploadTextureData(textureResources[0], mipImages[0], device,
  //     commandList);

  // mipImages[1] = LoadTexture(modelData.material.textureFilePath);
  //// メタデータ取得
  // metadata[1] = mipImages[1].GetMetadata();
  //// GPU 上のテクスチャリソースを作成
  // textureResources[1] = CreateTextureResource(device, metadata[1]);
  //// サブリソースをアップロードしてインターメディエイトバッファを取得
  // intermediateResources[1] =
  //     UploadTextureData(textureResources[1], mipImages[1], device,
  //     commandList);

  //// SRV をまとめて作るための配列
  // D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU[kNumTextures];
  // D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU[kNumTextures];

  //// SRV の設定と生成をループで
  // for (int i = 0; i < kNumTextures; ++i) {
  //   // ----------------------
  //   // SRV のディスクリプタ記述子を作成
  //   D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  //   srvDesc.Format = metadata[i].format;
  //   srvDesc.Shader4ComponentMapping =
  //   D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; srvDesc.ViewDimension =
  //   D3D12_SRV_DIMENSION_TEXTURE2D; srvDesc.Texture2D.MipLevels =
  //   static_cast<UINT>(metadata[i].mipLevels);

  //  // ----------------------
  //  // ヒープ上のスロットを決める（ImGui 用が 0 使っているので +1）
  //  UINT descriptorIndex = 1 + i;
  //  textureSrvHandleCPU[i] = GetCPUDescriptorHandle(
  //      srvDescriptorHeap, descriptorSizeSRV, descriptorIndex);
  //  textureSrvHandleGPU[i] = GetGPUDescriptorHandle(
  //      srvDescriptorHeap, descriptorSizeSRV, descriptorIndex);

  //  // ----------------------
  //  // SRV を生成
  //  device->CreateShaderResourceView(
  //      textureResources[i].Get(), // CreateTextureResource で作ったリソース
  //      &srvDesc, textureSrvHandleCPU[i]);
  //}

  ///// =============================================
  /////
  ///// ビューポートとシザーの設定
  /////
  ///// =============================================

  ////// ビューポート
  ////D3D12_VIEWPORT viewport{};
  ////// クライアント領域のサイズと一緒にして画面全体に表示
  ////viewport.Width = WinApp::kClientWidth;
  ////viewport.Height = WinApp::kClientHeight;
  ////viewport.TopLeftX = 0;
  ////viewport.TopLeftY = 0;
  ////viewport.MinDepth = 0.0f;
  ////viewport.MaxDepth = 1.0f;

  ////// シザー矩形
  ////D3D12_RECT scissorRect{};
  ////// 基本的にビューポートと同じ矩形が構成されるようにする
  ////scissorRect.left = 0;
  ////scissorRect.right = WinApp::kClientWidth;
  ////scissorRect.top = 0;
  ////scissorRect.bottom = WinApp::kClientHeight;

  /// =============================================
  ///
  /// サウンドの初期化処理
  ///
  /// =============================================

  SoundManager soundManager;
  soundManager.Initialize();

  Sound alarmSound;
  alarmSound.LoadWav("resources/Alarm01.wav");
  alarmSound.Play(&soundManager);

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

  // =============================================
  //
  // モデルを読み込む（plane.obj）
  //
  // =============================================

  ModelData modelData = LoadObjFile("resources", "plane.obj");

  // 頂点バッファ用リソース作成
  ComPtr<ID3D12Resource> vertexResourceModel = dxCommon->CreateBufferResource(
      sizeof(VertexData) * modelData.vertices.size());

  // 頂点バッファビューを作成
  D3D12_VERTEX_BUFFER_VIEW vertexBufferViewModel{};
  vertexBufferViewModel.BufferLocation =
      vertexResourceModel->GetGPUVirtualAddress();
  vertexBufferViewModel.SizeInBytes =
      UINT(sizeof(VertexData) * modelData.vertices.size());
  vertexBufferViewModel.StrideInBytes = sizeof(VertexData);

  // リソースに頂点データを書き込み
  VertexData *vertexDataModel = nullptr;
  vertexResourceModel->Map(0, nullptr,
                           reinterpret_cast<void **>(&vertexDataModel));
  std::memcpy(vertexDataModel, modelData.vertices.data(),
              sizeof(VertexData) * modelData.vertices.size());

  // TransformMatrix（World, WVP）用定数バッファ
  ComPtr<ID3D12Resource> transformMatrixResourceModel =
      dxCommon->CreateBufferResource(sizeof(TransformMatrix));

  TransformMatrix *transformMatrixDataModel = nullptr;
  transformMatrixResourceModel->Map(
      0, nullptr, reinterpret_cast<void **>(&transformMatrixDataModel));
  transformMatrixDataModel->World = MakeIdentityMatrix();
  transformMatrixDataModel->WVP = MakeIdentityMatrix();

  // モデル用 Transform
  Transform transformModel{
      {1.0f, 1.0f, 1.0f}, // scale
      {0.0f, 0.0f, 0.0f}, // rotate
      {0.0f, 0.0f, 0.0f}  // translate
  };

  // カメラ
  Transform transformCamera{
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f}};

  // マテリアル用定数バッファ
  ComPtr<ID3D12Resource> materialResource =
      dxCommon->CreateBufferResource(sizeof(Material));
  Material *materialData = nullptr;
  materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));
  materialData->color = {1.0f, 1.0f, 1.0f, 1.0f};
  materialData->enableLighting = 1;
  materialData->uvTransform = MakeIdentityMatrix();

  // ライト用定数バッファ
  ComPtr<ID3D12Resource> directionalLightResource =
      dxCommon->CreateBufferResource(sizeof(DirectionalLight));
  DirectionalLight *directionalLightData = nullptr;
  directionalLightResource->Map(
      0, nullptr, reinterpret_cast<void **>(&directionalLightData));
  directionalLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
  directionalLightData->direction = {0.0f, -1.0f, 0.0f};
  directionalLightData->intensity = 1.0f;

  DirectX::ScratchImage mipImages =
      dxCommon->LoadTexture(modelData.material.textureFilePath);
  DirectX::TexMetadata metadata = mipImages.GetMetadata();

  ComPtr<ID3D12Resource> textureResource =
      dxCommon->CreateTextureResource(metadata);

  dxCommon->UploadTextureData(textureResource, mipImages);

  // SRV 設定
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = metadata.format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = static_cast<UINT>(metadata.mipLevels);

  // ImGui が SRV の 0 番を使っている想定なので、1 番に割り当て
  D3D12_CPU_DESCRIPTOR_HANDLE textureSrvCPU =
      dxCommon->GetSRVCPUDescriptorHandle(1);
  D3D12_GPU_DESCRIPTOR_HANDLE textureSrvGPU =
      dxCommon->GetSRVGPUDescriptorHandle(1);

  dxCommon->GetDevice()->CreateShaderResourceView(textureResource.Get(),
                                                  &srvDesc, textureSrvCPU);





  //// DirectInputの初期化
  // IDirectInput8 *directInput = nullptr;
  // hr = DirectInput8Create(wc.hInstance, DIRECTINPUT_VERSION,
  // IID_IDirectInput8,
  //                         (void **)&directInput, nullptr);

  // assert(SUCCEEDED(hr));

  //// キーボードデバイスの生成
  // IDirectInputDevice8 *keyboard = nullptr;
  // hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
  // assert(SUCCEEDED(hr));

  //// 入力データ形式セット
  // hr = keyboard->SetDataFormat(&c_dfDIKeyboard);
  // assert(SUCCEEDED(hr));

  //// 排他制御レベルのセット
  // hr = keyboard->SetCooperativeLevel(
  //     hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
  // assert(SUCCEEDED(hr));

  /// =============================================
  ///
  /// ImGuiの初期化
  ///
  /// =============================================

  /*IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplWin32_Init(winApp->GetHwnd());
  ImGui_ImplDX12_Init(device.Get(), swapChainDesc.BufferCount, rtvDesc.Format,
                      srvDescriptorHeap.Get(),
                      srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                      srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());*/

  /// =============================================
  ///
  /// 変数の宣言と初期化
  ///
  /// =============================================

  // Transform transform{
  //     {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  // Transform transformCamera{
  //     {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f}};

  //// UI
  // Transform transformSprite{
  //     {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  //// 球
  // Matrix4x4 worldMatrix{};
  // Matrix4x4 cameraMatrix{};
  // Matrix4x4 viewMatrix{};
  // Matrix4x4 projectionMatrix{};
  // Matrix4x4 worldViewProjectionMatrix{};

  //// UI
  // Matrix4x4 worldMatrixSprite{};
  // Matrix4x4 viewMatrixSprite{};
  // Matrix4x4 projectionMatrixSprite{};
  // Matrix4x4 worldViewProjectionMatrixSprite{};

  // Transform uvTransformSprite{
  //     {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  // Matrix4x4 uvTransformMatrix{};

  //// SRVの切り替え
  // bool useMonsterBall = true;

  // Transform transformModel{
  //     {1.0f, 1.0f, 1.0f}, // scale
  //     {0.0f, 0.0f, 0.0f}, // rotate (radian)
  //     {0.0f, 0.0f, 0.0f}  // translate
  // };

  Vector2 scale = sprite->GetScale();

  // ウィンドウの×ボタンが押されるまでループ
  while (true) {

    if (winApp->ProcessMessage()) {
      // ゲームループを終了
      break;
    }

    // 入力更新
    input->Update();

    // ゲームの処理

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // ImGui::ShowDemoWindow();

    ImGui::Begin("Window");

    //// 色を変更
    // ImGui::ColorEdit3("Color", materialColor);
    //// 変更後の materialColorを定数バッファに書き戻す
    // materialData->color = Vector4{
    //     materialColor[0], materialColor[1], materialColor[2],
    //     materialData->color.w // αは前のまま残すか、1.0f にしちゃってもOK
    // };
    //// ライティングのON/OFFもImGuiでいじりたいなら…
    // static bool lightingOn = true;
    // ImGui::Checkbox("Enable Lighting", &lightingOn);
    // materialData->enableLighting = lightingOn ? 1 : 0;

    //// 既存の ImGui::Begin("Window"); の中あたりに…
    // ImGui::Separator();
    // ImGui::Text("Camera");
    // ImGui::DragFloat3("Camera translation", &transformCamera.translate.x,
    // 0.1f); ImGui::SliderAngle("Camera rotation X",
    // &transformCamera.rotate.x); ImGui::SliderAngle("Camera rotation Y",
    // &transformCamera.rotate.y); ImGui::SliderAngle("Camera rotation Z",
    // &transformCamera.rotate.z);

    ImGui::Separator();
    ImGui::Text("Model");

    ImGui::DragFloat3("Model Scale", &transformModel.scale.x, 0.01f, 0.01f,
                      10.0f);

    ImGui::SliderAngle("Model Rot X", &transformModel.rotate.x, -180.0f,
                       180.0f);
    ImGui::SliderAngle("Model Rot Y", &transformModel.rotate.y, -180.0f,
                       180.0f);
    ImGui::SliderAngle("Model Rot Z", &transformModel.rotate.z, -180.0f,
                       180.0f);

    ImGui::DragFloat3("Model Pos", &transformModel.translate.x, 0.1f, -100.0f,
                      100.0f);

    // ─────────────────────
    // カメラ
    // ─────────────────────
    ImGui::Separator();
    ImGui::Text("Camera");

    ImGui::DragFloat3("Camera Pos", &transformCamera.translate.x, 0.1f, -100.0f,
                      100.0f);

    ImGui::SliderAngle("Camera Rot X", &transformCamera.rotate.x, -180.0f,
                       180.0f);
    ImGui::SliderAngle("Camera Rot Y", &transformCamera.rotate.y, -180.0f,
                       180.0f);
    ImGui::SliderAngle("Camera Rot Z", &transformCamera.rotate.z, -180.0f,
                       180.0f);

    // ─────────────────────
    // マテリアル・ライト
    // ─────────────────────
    ImGui::Separator();
    ImGui::Text("Material / Light");

    static float materialColor[4] = {1, 1, 1, 1};
    ImGui::ColorEdit4("Color", materialColor);
    materialData->color = {materialColor[0], materialColor[1], materialColor[2],
                           materialColor[3]};

    static bool lightingOn = true;
    ImGui::Checkbox("Enable Lighting", &lightingOn);
    materialData->enableLighting = lightingOn ? 1 : 0;

    ImGui::ColorEdit3("Light Color", &directionalLightData->color.x);
    ImGui::DragFloat3("Light Dir", &directionalLightData->direction.x, 0.01f,
                      -1.0f, 1.0f);
    ImGui::DragFloat("Light Intensity", &directionalLightData->intensity, 0.01f,
                     0.0f, 10.0f);

    // ライトの方向正規化（方向ベクトルが変な長さにならないように）
    {
      Vector3 &d = directionalLightData->direction;
      float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
      if (len > 0.0f) {
        d.x /= len;
        d.y /= len;
        d.z /= len;
      }
    }

    //// 球体の拡縮、回転、移動
    // ImGui::Separator();
    // ImGui::Text("Sphere");
    // ImGui::DragFloat3("scale", &transform.scale.x, 0.01f);
    // ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);
    // ImGui::DragFloat3("translate", &transform.translate.x, 0.01f);

    // ImGui::Spacing();

    //// UIの移動
    // ImGui::Text("UI");
    // ImGui::SliderFloat3("translationSprite", &transformSprite.translate.x,
    // 0.0f,
    //                     1280.0f);
    // ImGui::DragFloat3("scaleSprite", &transformSprite.scale.x, 0.01f);
    // ImGui::DragFloat3("rotationSprite", &transformSprite.rotate.x, 0.01f);

    ///*ImGui::Checkbox("useMonsterBall", &useMonsterBall);*/

    // ImGui::Separator();
    // ImGui::Text("UVTransform");
    // ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f,
    //                   -10.0f, 10.0f);
    // ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f,
    //                   10.0f);
    // ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);

    //// ライト
    // ImGui::Separator();
    // ImGui::Text("Directional Light");
    // ImGui::ColorEdit3("Light Color", &directionalLightData->color.x);
    // ImGui::DragFloat3("Direction", &directionalLightData->direction.x, 0.01f,
    //                   -1.0f, 1.0f);

    //// ライトの正規化
    // Vector3 &direction = directionalLightData->direction;
    // float length =
    //     std::sqrt(direction.x * direction.x + direction.y * direction.y +
    //               direction.z * direction.z);
    // if (length > 0.0f) {
    //   direction.x /= length;
    //   direction.y /= length;
    //   direction.z /= length;
    // }

    // ImGui::DragFloat("Intensity", &directionalLightData->intensity, 0.01f,
    // 0.0f,
    //                  10.0f);
    ImGui::End();

    // ComPtr<ID3D12InfoQueue> infoQueue;
    // if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
    //   UINT64 count =
    //   infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter(); for (UINT64
    //   i = 0; i < count; ++i) {
    //     SIZE_T msgLen = 0;
    //     infoQueue->GetMessage(i, nullptr, &msgLen);
    //     auto buffer = (D3D12_MESSAGE *)malloc(msgLen);
    //     infoQueue->GetMessage(i, buffer, &msgLen);
    //     OutputDebugStringA(buffer->pDescription); // 何が NG なのかを出力
    //     free(buffer);
    //   }
    //   infoQueue->ClearStoredMessages();
    // }

    /////
    ///// 更新処理 ↓
    /////

    scale += Vector2(0.1f, 0.1f);
    sprite->SetScale(scale);

    sprite->Update();

    // カメラ行列
    Matrix4x4 cameraMatrix =
        MakeAffineMatrix(transformCamera.scale, transformCamera.rotate,
                         transformCamera.translate);
    Matrix4x4 viewMatrix = MakeInverseMatrix(cameraMatrix);
    Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
        0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f,
        100.0f);

    // モデルの World / WVP
    Matrix4x4 worldModel = MakeAffineMatrix(
        transformModel.scale, transformModel.rotate, transformModel.translate);
    Matrix4x4 wvpModel =
        Multiply(worldModel, Multiply(viewMatrix, projectionMatrix));

    transformMatrixDataModel->World = worldModel;
    transformMatrixDataModel->WVP = wvpModel;

    ///*if (isTrigger(DIK_SPACE, keys, preKeys)) {
    //  transform.translate.x += 2.0f;
    //}*/

    //// 三角形
    // worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate,
    //                                transform.translate);
    // cameraMatrix =
    //     MakeAffineMatrix(transformCamera.scale, transformCamera.rotate,
    //                      transformCamera.translate);
    // viewMatrix = Inverse(cameraMatrix);
    // projectionMatrix = MakePerspectiveFovMatrix(
    //     0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight),
    //     0.1f, 100.0f);
    // transformMatrixData->World = worldMatrix;
    // transformMatrixData->WVP =
    //     Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

    //// UI
    // worldMatrixSprite =
    //     MakeAffineMatrix(transformSprite.scale, transformSprite.rotate,
    //                      transformSprite.translate);
    // viewMatrixSprite = MakeIdentityMatrix();
    // projectionMatrixSprite = MakeOrthographicMatrix(
    //     0.0f, 0.0f, static_cast<float>(WinApp::kClientWidth),
    //     static_cast<float>(WinApp::kClientHeight), 0.0f, 100.0f);
    // worldViewProjectionMatrixSprite = Multiply(
    //     worldMatrixSprite, Multiply(viewMatrixSprite,
    //     projectionMatrixSprite));
    // transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;

    // uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
    // uvTransformMatrix = Multiply(uvTransformMatrix,
    //                              MakeRotateZMatrix(uvTransformSprite.rotate.z));
    // uvTransformMatrix = Multiply(
    //     uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
    // materialDataSprite->uvTransform = uvTransformMatrix;

    // Matrix4x4 worldModel = MakeAffineMatrix(
    //     transformModel.scale, transformModel.rotate,
    //     transformModel.translate);
    // Matrix4x4 wvpModel =
    //     Multiply(worldModel, Multiply(viewMatrix, projectionMatrix));

    // transformMatrixDataModel->World = worldModel;
    // transformMatrixDataModel->WVP = wvpModel;

    /////
    ///// 更新処理 ↑
    /////

    //// これから書き込むバックバッファのインデックスを取得
    // UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

    //// TransitionBarrierの設定
    // D3D12_RESOURCE_BARRIER barrier{};
    //// 今回のバリアはTransition
    // barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    //// Noneにしておく
    // barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    //// バリアを張る対象のリソース、現在のバックバッファに対して行う
    // barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
    //// 遷移前のResourceState
    // barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    //// 遷移後のResourceState
    // barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    //// TransitionBarrierを張る
    // commandList->ResourceBarrier(1, &barrier);

    //// 描画先のRTVとDSVを設定する
    // D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
    //     dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    // commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false,
    //                                 &dsvHandle);
    //// 指定した深度で画面全体をクリアする
    // commandList->ClearDepthStencilView(dsvHandle,
    // D3D12_CLEAR_FLAG_DEPTH, 1.0f,
    //                                    0, 0, nullptr);

    //// 指定した色で画面全体をクリアする
    // float clearColor[] = {0.1f, 0.25f, 0.5f, 1.0f}; // RGBAの順
    // commandList->ClearRenderTargetView(rtvHandles[backBufferIndex],
    // clearColor,
    //                                    0, nullptr);

    ImGui::Render();

    //// 描画用のDescriptorHeapの設定
    // ID3D12DescriptorHeap *descriptorHeaps[] = {srvDescriptorHeap.Get()};
    // commandList->SetDescriptorHeaps(1, descriptorHeaps);a

    /////
    ///// 描画処理 ↓
    /////

    dxCommon->BeginDraw();

    sprite->Draw();

    // commandList->RSSetViewports(1, &viewport);       // Viewportを設定
    // commandList->RSSetScissorRects(1, &scissorRect); // Scirssorを設定

    //// RootSignatureを設定、PSOに設定しているけど別途設定が必要
    // commandList->SetGraphicsRootSignature(rootSignature.Get());
    // commandList->SetPipelineState(graphicsPipelineState.Get()); // PSOを設定
    // commandList->IASetVertexBuffers(0, 1, &vertexBufferView);   // VBVを設定
    //// 形状を設定、PSOに設定しているものとはまた別のもの
    // commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //// マテリアルのCBufferの場所を設定
    // commandList->SetGraphicsRootConstantBufferView(
    //     0, materialResource->GetGPUVirtualAddress());
    //// wvp用のCBufferの場所を設定
    // commandList->SetGraphicsRootConstantBufferView(
    //     1, transformMatrixResource->GetGPUVirtualAddress());
    // commandList->SetGraphicsRootDescriptorTable(
    //     2, useMonsterBall ? textureSrvHandleGPU[1] : textureSrvHandleGPU[0]);

    //// ライト用のCBufferの場所を設定
    // commandList->SetGraphicsRootConstantBufferView(
    //     3, directionalLightResource->GetGPUVirtualAddress());

    //// 球の描画
    // commandList->DrawInstanced(kSubdivision * kSubdivision * 6, 1, 0, 0);

    //// マテリアルのCBufferの場所を設定
    // commandList->SetGraphicsRootConstantBufferView(
    //     0, materialResourceSprite->GetGPUVirtualAddress());
    //// SpriteのSRVをuvCheckerに
    // commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU[0]);

    //// Sprite用の描画
    //// 頂点データの入っているバッファ
    // commandList->IASetVertexBuffers(0, 1,
    //                                 &vertexBufferViewSprite); // VBVを設定
    //// インデックスバッファ
    // commandList->IASetIndexBuffer(&indexBufferViewSprite);

    //// TransformationMatrixCBufferの場所を設定
    // commandList->SetGraphicsRootConstantBufferView(
    //     1, transformationMatrixResourceSprite->GetGPUVirtualAddress());

    //// UI(2D)描画
    // commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

    // commandList->SetGraphicsRootConstantBufferView(
    //     0, materialResource->GetGPUVirtualAddress());

    // commandList->SetGraphicsRootConstantBufferView(
    //     1, transformMatrixResourceModel->GetGPUVirtualAddress());

    //// 頂点バッファビューをモデル用に切り替え
    // commandList->IASetVertexBuffers(0, 1, &vertexBufferViewModel);

    // commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

    // ★ ImGui / 自前テクスチャ用 SRV ヒープをバインド
    ID3D12DescriptorHeap *descriptorHeaps[] = {dxCommon->GetSRVHeap()};
    dxCommon->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);

    // ★ パイプラインの設定
    /*dxCommon->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
    dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());*/

    spriteCommon->SetupCommonRenderState();

    // 頂点バッファ（plane.obj）
    dxCommon->GetCommandList()->IASetVertexBuffers(0, 1,
                                                   &vertexBufferViewModel);
    /*dxCommon->GetCommandList()->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);*/

    // マテリアル CBV
    dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(
        0, materialResource->GetGPUVirtualAddress());

    // TransformMatrix（World/WVP）CBV
    dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(
        1, transformMatrixResourceModel->GetGPUVirtualAddress());

    // テクスチャ SRV（plane テクスチャ）
    dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2,
                                                               textureSrvGPU);

    // ライト CBV
    dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(
        3, directionalLightResource->GetGPUVirtualAddress());

    // ★ plane.obj を描画
    dxCommon->GetCommandList()->DrawInstanced(
        static_cast<UINT>(modelData.vertices.size()), 1, 0, 0);

    // 実際のcommandListのImGuiの描画コマンドを積む
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),
                                  dxCommon->GetCommandList());

    dxCommon->EndDraw();

    /////
    ///// 描画処理 ↑
    /////

    //// 今回はRenderTargetからPresentにする
    // barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    // barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    //// TrasitionBarrierを張る
    // commandList->ResourceBarrier(1, &barrier);

    //// コマンドリストの内容を確定させる、すべてのコマンドを積んでからCloseする
    // hr = commandList->Close();
    // assert(SUCCEEDED(hr));

    //// GPUにコマンドリストの実行を行わせる(キックする)
    // ID3D12CommandList *commandLists[] = {commandList.Get()};
    // commandQueue->ExecuteCommandLists(1, commandLists);
    //// GPUとOSに画面の交換を行うように通知する
    // swapChain->Present(1, 0);

    //// Fenceの値を更新
    // fenceValue++;
    ////
    ///GPUがここまでたどり着いたときにFenceの値を指定した値に代入するようにSignalを送る
    // commandQueue->Signal(fence.Get(), fenceValue);

    //// Fenceの値が指定したSignal値にたどり着いているのかを確認
    // if (fence->GetCompletedValue() < fenceValue) {
    //   //
    //   指定したSignalにたどりついていないので、たどりつくまで待つようにイベントを設定
    //   fence->SetEventOnCompletion(fenceValue, fenceEvent);
    //   // イベントを待つ
    //   WaitForSingleObject(fenceEvent, INFINITE);
    // }

    //// 次のフレーム用のコマンドリストを準備
    // hr = commandAllocator->Reset();
    // assert(SUCCEEDED(hr));
    // hr = commandList->Reset(commandAllocator.Get(), nullptr);
    // assert(SUCCEEDED(hr));
  }

  // ImGuiの終了処理、初期化と逆順に行う
  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  //// 解放処理
  // CloseHandle(fenceEvent);

  ///*CloseWindow(winApp->GetHwnd());*/

  // signatureBlob->Release();

  // if (errorBlob) {
  //   errorBlob->Release();
  // }

  // pixelShaderBlob->Release();
  // vertexShaderBlob->Release();

  // if (includeHandler) {
  //   includeHandler->Release();
  // }

  // if (dxcCompiler) {
  //   dxcCompiler->Release();
  // }

  // if (dxcUtils) {
  //   dxcUtils->Release();
  // }

  // inputを解放
  delete input;

  //spriteを解放
  delete sprite;

  //spriteCommonを解放
  delete spriteCommon;

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