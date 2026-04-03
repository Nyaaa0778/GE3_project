#include "AudioManager.h"
#include "StringUtility.h"

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

using namespace StringUtility;
using namespace Microsoft::WRL;

std::unique_ptr<AudioManager> AudioManager::instance = nullptr;

AudioManager* AudioManager::GetInstance() {
    if (instance == nullptr) {
        instance = std::make_unique<AudioManager>();
    }
    return instance.get();
}

void AudioManager::Finalize() { instance.reset(); }

AudioManager::~AudioManager() {
    Release();
}

void AudioManager::Initialize() {
    HRESULT hr;
    hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    assert(SUCCEEDED(hr));

    hr = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
    assert(SUCCEEDED(hr));

    hr = xAudio2_->CreateMasteringVoice(&masterVoice_);
    assert(SUCCEEDED(hr));
}

void AudioManager::Release() {
    Unload();

    if (masterVoice_) {
        masterVoice_->DestroyVoice();
        masterVoice_ = nullptr;
    }
    xAudio2_.Reset();

    MFShutdown();
}

void AudioManager::LoadAudio(const std::string& filepath) {
    // 既に読み込み済みの場合は何もしない（二重読み込み防止）
    if (audioDatas_.find(filepath) != audioDatas_.end()) {
        return;
    }

    std::string fullPath = "resources/sounds/" + filepath;
    std::wstring filePathW = ConvertString(fullPath.c_str());

    AudioData soundData;
    if (ExtractAudioData(filePathW, soundData)) {
        // 読み込んだデータを辞書に登録
        audioDatas_[filepath] = std::move(soundData);
    }
}

void AudioManager::Unload() {
    audioDatas_.clear();
}

const AudioData* AudioManager::GetAudioData(const std::string& filepath) const {
    auto it = audioDatas_.find(filepath);
    if (it != audioDatas_.end()) {
        return &it->second; // データが見つかったらポインタを返す
    }
    return nullptr;
}

//================================================================================
// 内部ヘルパー
//================================================================================
bool AudioManager::ExtractAudioData(const std::wstring& filePath, AudioData& outData) {
    HRESULT hr;

    // 1. SourceReaderの作成
    ComPtr<IMFSourceReader> pReader;
    hr = MFCreateSourceReaderFromURL(filePath.c_str(), nullptr, &pReader);
    assert(SUCCEEDED(hr));

    // 2. メディアタイプをPCMに指定
    ComPtr<IMFMediaType> pPCMType;
    MFCreateMediaType(&pPCMType);
    pPCMType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    pPCMType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    hr = pReader->SetCurrentMediaType((DWORD) MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pPCMType.Get());

    // 3. 実際のフォーマットを取得
    ComPtr<IMFMediaType> pOutType;
    pReader->GetCurrentMediaType((DWORD) MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pOutType);

    WAVEFORMATEX* waveFormat = nullptr;
    MFCreateWaveFormatExFromMFMediaType(pOutType.Get(), &waveFormat, nullptr);
    outData.waveFormat = *waveFormat;

    // 不要になったフォーマット用メモリを解放
    CoTaskMemFree(waveFormat);

    // 4. サンプルデータの読み込みループ
    while (true) {
        ComPtr<IMFSample> pSample;
        DWORD streamIndex = 0;
        DWORD flags = 0;
        LONGLONG llTimeStamp = 0;

        // サンプルの読み込み
        hr = pReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &streamIndex, &flags, &llTimeStamp, &pSample);

        // ストリームの末端なら終了
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
            break;
        }

        if (pSample) {
            ComPtr<IMFMediaBuffer> pBuffer;
            pSample->ConvertToContiguousBuffer(&pBuffer);

            BYTE* pData = nullptr;
            DWORD maxLength = 0;
            DWORD currentLength = 0;

            // バッファをロックしてデータを抽出
            pBuffer->Lock(&pData, &maxLength, &currentLength);
            outData.audioBuffer.insert(outData.audioBuffer.end(), pData, pData + currentLength);
            pBuffer->Unlock();
        }
    }

    return true;
}