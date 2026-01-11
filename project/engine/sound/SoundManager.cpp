#include "SoundManager.h"

#include "StringUtility.h"

using namespace StringUtility;
using namespace Microsoft::WRL;

#include <cassert>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <combaseapi.h>

#pragma warning(push)
#pragma warning(disable : 4229)
#include <mfapi.h>
#include <mfidl.h> 
#include <mfreadwrite.h>
#pragma warning(pop)

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "ole32.lib")
SoundManager::SoundManager() = default;

SoundManager::~SoundManager() { Finalize(); }

void SoundManager::Initialize() {
  HRESULT result;

  // Windows Media Foundationの初期化
  result = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
  assert(SUCCEEDED(result));

  // XAudio2の初期化
  result = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
  assert(SUCCEEDED(result));

  result = xAudio2_->CreateMasteringVoice(&masterVoice_);
  assert(SUCCEEDED(result));
}

void SoundManager::Finalize() {

  Unload();

  if (masterVoice_) {
    masterVoice_->DestroyVoice();
    masterVoice_ = nullptr;
  }
  xAudio2_.Reset();

  HRESULT result;

  // Windows Media Foundationの終了
  result = MFShutdown();
  assert(SUCCEEDED(result));
}

/// <summary>
/// 音声データを読み込む
/// </summary>
/// <param name="filename"></param>
/// <returns></returns>
SoundData SoundLoadFile(const char *filePath) {

  // 古パスをワイド文字列に変換
  std::wstring filePathW = ConvertString(filePath);
  HRESULT result;

  // SourceReader作成
  ComPtr<IMFSourceReader> pReader;
  result = MFCreateSourceReaderFromURL(filePathW.c_str(), nullptr, &pReader);
  assert(SUCCEEDED(result));

  // PCM形式にフォーマット指定する
  ComPtr<IMFMediaType> pPCMType;
  MFCreateMediaType(&pPCMType);
  pPCMType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
  pPCMType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
  result = pReader->SetCurrentMediaType(
      (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pPCMType.Get());

  // 実際にセットされたメディアタイプを取得する
  ComPtr<IMFMediaType> pOutType;
  pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                               &pOutType);

  // Waveフォーマットを取得する
  WAVEFORMATEX *waveFormat = nullptr;
  MFCreateWaveFormatExFromMFMediaType(pOutType.Get(), &waveFormat, nullptr);

  // コンテナに格納する音声データ
  SoundData soundData = {};
  soundData.wfex = *waveFormat;

  // 生成したWaveフォーマットを解放
  CoTaskMemFree(waveFormat);

  // PCMデータのバッファを構築
  while (true) {
    ComPtr<IMFSample> pSample;
    DWORD streamIndex = 0;
    DWORD flags = 0;
    LONGLONG llTImeStamp = 0;
    // サンプルを読み込む
    result = pReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0,
                                 &streamIndex, &flags, &llTImeStamp, &pSample);
    // ストリームの末尾に達したら抜ける
    if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
      break;
    }

    if (pSample) {
      ComPtr<IMFMediaBuffer> pBuffer;
      // サンプルに含まれるサウンドデータのバッファを一繋ぎにして取得
      pSample->ConvertToContiguousBuffer(&pBuffer);
      // データ読み取り用のポインタ
      BYTE *pData = nullptr;
      DWORD maxLength = 0;
      DWORD currentLength = 0;
      // バッファ読み込み用にロック
      pBuffer->Lock(&pData, &maxLength, &currentLength);
      // バッファの末尾にデータを追加
      soundData.buffer.insert(soundData.buffer.end(), pData,
                              pData + currentLength);
      pBuffer->Unlock();
    }
  }
  return soundData;
}

/// <summary>
/// 音声データの解放
/// </summary>
/// <param name="data"></param>
void SoundUnload(SoundData *data) {
  data->buffer.clear();
  data->wfex = {};
}

bool SoundManager::Load(const std::string &filepath) {
  soundData_ = SoundLoadFile(filepath.c_str());
  return true;
}

/// <summary>
/// 音声再生
/// </summary>
/// <param name="manager"></param>
void SoundManager::Play(SoundManager *manager) {
  HRESULT result;
  result =
      manager->GetXAudio2()->CreateSourceVoice(&sourceVoice_, &soundData_.wfex);
  assert(SUCCEEDED(result));

  XAUDIO2_BUFFER buf{};
  buf.pAudioData = soundData_.buffer.data();
  buf.AudioBytes = static_cast<UINT32>(soundData_.buffer.size());
  buf.Flags = XAUDIO2_END_OF_STREAM;

  result = sourceVoice_->SubmitSourceBuffer(&buf);
  assert(SUCCEEDED(result));
  result = sourceVoice_->Start();
  assert(SUCCEEDED(result));
}

void SoundManager::Unload() {
  if (sourceVoice_) {
    sourceVoice_->DestroyVoice();
    sourceVoice_ = nullptr;
  }
  SoundUnload(&soundData_);
}