#include "AudioManager.h"
#include "StringUtility.h"

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
#pragma comment(lib, "xaudio2.lib")

#include <cassert>

using namespace StringUtility;
using namespace Microsoft::WRL;

//================================================================================
// シングルトン
//================================================================================

std::unique_ptr<AudioManager> AudioManager::instance = nullptr;

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>AudioManager の唯一のインスタンス</returns>
AudioManager* AudioManager::GetInstance() {
	if (instance == nullptr) {
		instance = std::make_unique<AudioManager>();
	}
	return instance.get();
}

/// <summary>
/// 終了
/// </summary>
void AudioManager::Finalize() { instance.reset(); }

/// <summary>
/// デストラクタ
/// </summary>
AudioManager::~AudioManager() {
	Release();
}

/// <summary>
/// 初期化
/// </summary>
void AudioManager::Initialize() {
	HRESULT hr;
	hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
	assert(SUCCEEDED(hr));

	hr = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(hr));

	hr = xAudio2_->CreateMasteringVoice(&masterVoice_);
	assert(SUCCEEDED(hr));

	// マスターボイスの設定を取得して、同じフォーマットでサブミックスを作る
	XAUDIO2_VOICE_DETAILS masterDetails;
	masterVoice_->GetVoiceDetails(&masterDetails);

	// BGM用サブミックスボイスの作成
	hr = xAudio2_->CreateSubmixVoice(&bgmSubmixVoice_, masterDetails.InputChannels, masterDetails.InputSampleRate);
	assert(SUCCEEDED(hr));

	// SE用サブミックスボイスの作成
	hr = xAudio2_->CreateSubmixVoice(&seSubmixVoice_, masterDetails.InputChannels, masterDetails.InputSampleRate);
	assert(SUCCEEDED(hr));
}

/// <summary>
/// 解放
/// </summary>
void AudioManager::Release() {
	UnloadInternal();

	// サブミックスボイスの破棄（必ずマスターボイスより前に破棄する）
	if (bgmSubmixVoice_) {
		bgmSubmixVoice_->DestroyVoice();
		bgmSubmixVoice_ = nullptr;
	}
	if (seSubmixVoice_) {
		seSubmixVoice_->DestroyVoice();
		seSubmixVoice_ = nullptr;
	}

	if (masterVoice_) {
		masterVoice_->DestroyVoice();
		masterVoice_ = nullptr;
	}
	xAudio2_.Reset();

	MFShutdown();
}

//================================================================================
// 音声データ管理 (静的メンバ関数)
//================================================================================

/// <summary>
/// 音声ファイルを読み込み、操作用のハンドル(ID)を返す
/// </summary>
uint32_t AudioManager::LoadAudio(const std::string& filepath, SoundGroup group) {
	return GetInstance()->LoadAudioInternal(filepath, group);
}

/// <summary>
/// メモリを解放
/// </summary>
void AudioManager::Unload() {
	GetInstance()->UnloadInternal();
}

//================================================================================
// 音声操作 (静的メンバ関数)
//================================================================================

// 再生
void AudioManager::PlayAudio(uint32_t handle, bool isLoop) {
	GetInstance()->PlayAudioInternal(handle, isLoop);
}
// ワンショット再生（同じ音を重複して鳴らす）
void AudioManager::PlayOneShot(uint32_t handle, float volume) {
	GetInstance()->PlayOneShotInternal(handle, volume); // 追加！
}
// 一時停止
void AudioManager::PauseAudio(uint32_t handle) {
	GetInstance()->PauseAudioInternal(handle);
}
// 再開
void AudioManager::ResumeAudio(uint32_t handle) {
	GetInstance()->ResumeAudioInternal(handle);
}
// 停止
void AudioManager::StopAudio(uint32_t handle) {
	GetInstance()->StopAudioInternal(handle);
}
// 音が鳴っているか？
bool AudioManager::IsPlaying(uint32_t handle) {
	return GetInstance()->IsPlayingInternal(handle);
}

// マスター音量
void AudioManager::SetMasterVolume(float volume) {
	GetInstance()->SetMasterVolumeInternal(volume);
}
// BGMの音量
void AudioManager::SetBGMVolume(float volume) {
	GetInstance()->SetBGMVolumeInternal(volume);
}
// SEの音量
void AudioManager::SetSEVolume(float volume) {
	GetInstance()->SetSEVolumeInternal(volume);
}
// ピッチ
void AudioManager::SetPitch(uint32_t handle, float ratio) {
	GetInstance()->SetPitchInternal(handle, ratio);
}
// 音のこもり具合
void AudioManager::SetMuffle(uint32_t handle, float percentage) {
	GetInstance()->SetMuffleInternal(handle, percentage);
}
// パンニング
void AudioManager::SetPan(uint32_t handle, float pan) {
	GetInstance()->SetPanInternal(handle, pan);
}

//================================================================================
// 音声データ管理
//================================================================================

/// <summary>
/// 音声ファイルを読み込み、操作用のハンドル(ID)を返す
/// </summary>
uint32_t AudioManager::LoadAudioInternal(const std::string& filepath, SoundGroup group) {
	// 既にロード済みの場合は、既存のハンドルを返す
	auto it = pathToHandleMap_.find(filepath);
	if (it != pathToHandleMap_.end()) {
		return it->second;
	}

	std::string fullPath = "resources/sounds/" + filepath;
	std::wstring filePathW = ConvertString(fullPath.c_str());

	AudioData soundData;
	if (!ExtractAudioData(filePathW, soundData)) {
		assert(false && "音声ファイルのロードに失敗しました");
		return 0; // 0を無効なハンドルとして返す
	}

	// 新しいハンドルを発行
	uint32_t newHandle = nextHandle_++;

	// グループを保存
	soundData.group = group;

	// データの登録
	audioDataMap_[newHandle] = std::move(soundData);
	pathToHandleMap_[filepath] = newHandle;

	// 出力先のサブミックスボイスを決定
	XAUDIO2_SEND_DESCRIPTOR sendDesc = {};
	sendDesc.Flags = 0;
	sendDesc.pOutputVoice = (group == SoundGroup::BGM) ? bgmSubmixVoice_ : seSubmixVoice_;

	XAUDIO2_VOICE_SENDS sendList = {};
	sendList.SendCount = 1;
	sendList.pSends = &sendDesc;

	// ボイス（スピーカー）の作成
	IXAudio2SourceVoice* sourceVoice = nullptr;
	// 第6引数に sendList を渡す
	HRESULT hr = xAudio2_->CreateSourceVoice(&sourceVoice, &audioDataMap_[newHandle].waveFormat, XAUDIO2_VOICE_USEFILTER, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, &sendList, nullptr);
	assert(SUCCEEDED(hr));

	voiceMap_[newHandle] = sourceVoice;

	return newHandle;
}

/// <summary>
/// メモリを解放
/// </summary>
void AudioManager::UnloadInternal() {
	for (auto& pair : voiceMap_) {
		if (pair.second) {
			pair.second->Stop();
			pair.second->DestroyVoice();
		}
	}

	// ワンショットボイスの破棄
	for (auto voice : oneShotVoices_) {
		if (voice) {
			voice->Stop();
			voice->DestroyVoice();
		}
	}
	oneShotVoices_.clear();
	voiceMap_.clear();
	audioDataMap_.clear();
	pathToHandleMap_.clear();
}

//================================================================================
// 音声操作
//================================================================================

/// <summary>
/// 再生
/// </summary>
/// <param name="handle">音声ハンドル</param>
/// <param name="isLoop">ループさせるか？</param>
void AudioManager::PlayAudioInternal(uint32_t handle, bool isLoop) {
	auto voiceIt = voiceMap_.find(handle);
	if (voiceIt == voiceMap_.end()) return; // ハンドルが無効なら何もしない

	IXAudio2SourceVoice* voice = voiceIt->second;
	auto& audioData = audioDataMap_[handle];

	voice->Stop();
	voice->FlushSourceBuffers();

	XAUDIO2_BUFFER buf {};
	buf.pAudioData = audioData.audioBuffer.data();
	buf.AudioBytes = static_cast<UINT32>(audioData.audioBuffer.size());
	buf.Flags = XAUDIO2_END_OF_STREAM;

	if (isLoop) {
		buf.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	HRESULT hr = voice->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(hr));

	hr = voice->Start();
	assert(SUCCEEDED(hr));
}

/// <summary>
/// ワンショット再生（同じ音を重複して鳴らす）
/// </summary>
/// <param name="handle">音声ハンドル</param>
/// <param name="volume">音量 (0.0f ~ 1.0f)</param>
void AudioManager::PlayOneShotInternal(uint32_t handle, float volume) {
	// 【追加】新しく再生する「ついで」に、再生終了した過去のボイスを検知して破棄する
	for (auto it = oneShotVoices_.begin(); it != oneShotVoices_.end();) {
		XAUDIO2_VOICE_STATE state;
		(*it)->GetState(&state);

		// キューに積まれているバッファが0になったら再生終了とみなす
		if (state.BuffersQueued == 0) {
			(*it)->DestroyVoice();
			it = oneShotVoices_.erase(it);
		} else {
			++it;
		}
	}

	// ハンドルから音声データを検索
	auto dataIt = audioDataMap_.find(handle);
	if (dataIt == audioDataMap_.end()) return;

	const AudioData& audioData = dataIt->second;

	// 出力先のサブミックスボイスを決定
	XAUDIO2_SEND_DESCRIPTOR sendDesc = {};
	sendDesc.Flags = 0;
	sendDesc.pOutputVoice = (audioData.group == SoundGroup::BGM) ? bgmSubmixVoice_ : seSubmixVoice_;

	XAUDIO2_VOICE_SENDS sendList = {};
	sendList.SendCount = 1;
	sendList.pSends = &sendDesc;

	// ワンショット用の新しいボイスを作成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	// 第6引数に sendList を渡す
	HRESULT hr = xAudio2_->CreateSourceVoice(&pSourceVoice, &audioData.waveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, &sendList, nullptr);
	if (FAILED(hr)) return;

	// 音量の設定
	volume = std::fmaxf(0.0f, std::fminf(1.0f, volume));
	pSourceVoice->SetVolume(volume);

	// バッファの設定
	XAUDIO2_BUFFER buf {};
	buf.pAudioData = audioData.audioBuffer.data();
	buf.AudioBytes = static_cast<UINT32>(audioData.audioBuffer.size());
	buf.Flags = XAUDIO2_END_OF_STREAM;
	buf.LoopCount = 0; // ワンショットなのでループしない

	// バッファを送信して再生開始
	hr = pSourceVoice->SubmitSourceBuffer(&buf);
	if (SUCCEEDED(hr)) {
		pSourceVoice->Start(0);
		// 管理リストに追加
		oneShotVoices_.push_back(pSourceVoice);
	} else {
		pSourceVoice->DestroyVoice();
	}
}

/// <summary>
/// 一時停止
/// </summary>
/// <param name="handle">音声ハンドル</param>
void AudioManager::PauseAudioInternal(uint32_t handle) {
	auto it = voiceMap_.find(handle);
	if (it != voiceMap_.end() && it->second) {
		it->second->Stop(0);
	}
}

/// <summary>
/// 再開
/// </summary>
/// <param name="handle">音声ハンドル</param>
void AudioManager::ResumeAudioInternal(uint32_t handle) {
	auto it = voiceMap_.find(handle);
	if (it != voiceMap_.end() && it->second) {
		it->second->Start(0);
	}
}

/// <summary>
/// 停止
/// </summary>
/// <param name="handle">音声ハンドル</param>
void AudioManager::StopAudioInternal(uint32_t handle) {
	auto it = voiceMap_.find(handle);
	if (it != voiceMap_.end() && it->second) {
		it->second->Stop();
		it->second->FlushSourceBuffers();
	}
}

/// <summary>
/// 音を再生中かどうか
/// </summary>
/// <param name="handle">音声ハンドル</param>
/// <returns></returns>
bool AudioManager::IsPlayingInternal(uint32_t handle) {
	auto it = voiceMap_.find(handle);
	if (it == voiceMap_.end() || !it->second) {
		return false;
	}

	XAUDIO2_VOICE_STATE state;
	it->second->GetState(&state);
	return state.BuffersQueued > 0;
}

/// <summary>
/// マスター音量（最終出力）の設定
/// </summary>
/// <param name="volume">音量 (0.0f ~ 1.0f)</param>
void AudioManager::SetMasterVolumeInternal(float volume) {
	if (masterVoice_) {
		// 0.0 ~ 1.0 の範囲にクランプ
		volume = std::fmaxf(0.0f, std::fminf(1.0f, volume));
		masterVoice_->SetVolume(volume);
	}
}

/// <summary>
/// BGMの音量
/// </summary>
/// <param name="volume">設定する音量（例: 0.0で無音、1.0で標準音量）</param>
void AudioManager::SetBGMVolumeInternal(float volume) {
	if (bgmSubmixVoice_) {
		volume = std::fmaxf(0.0f, std::fminf(1.0f, volume));
		bgmSubmixVoice_->SetVolume(volume);
	}
}

/// <summary>
/// SEの音量
/// </summary>
/// <param name="volume">設定する音量（例: 0.0で無音、1.0で標準音量）</param>
void AudioManager::SetSEVolumeInternal(float volume) {
	if (seSubmixVoice_) {
		volume = std::fmaxf(0.0f, std::fminf(1.0f, volume));
		seSubmixVoice_->SetVolume(volume);
	}
}

/// <summary>
/// ピッチ
/// </summary>
/// <param name="handle">音声ハンドル</param>
/// <param name="ratio">ピッチ（音程・再生速度）の倍率（例: 1.0で標準、2.0で1オクターブ上、0.5で1オクターブ下）</param>
void AudioManager::SetPitchInternal(uint32_t handle, float ratio) {
	auto it = voiceMap_.find(handle);
	if (it != voiceMap_.end() && it->second) {
		it->second->SetFrequencyRatio(ratio);
	}
}

/// <summary>
/// 音のこもり具合
/// </summary>
/// <param name="handle">音声ハンドル</param>
/// <param name="percentage">こもり具合（ローパスフィルター等）の割合（例: 0.0でクリア、1.0で最大）</param>
void AudioManager::SetMuffleInternal(uint32_t handle, float percentage) {
	auto it = voiceMap_.find(handle);
	if (it == voiceMap_.end() || !it->second) return;

	percentage = std::fmaxf(0.0f, std::fminf(1.0f, percentage));

	XAUDIO2_FILTER_PARAMETERS params;
	params.Type = LowPassFilter;
	params.Frequency = std::powf(1.0f - percentage, 3.0f);
	if (params.Frequency < 0.01f) params.Frequency = 0.01f;
	params.OneOverQ = 1.0f;

	it->second->SetFilterParameters(&params);
}

/// <summary>
/// パンニング
/// </summary>
/// <param name="handle">音声ハンドル</param>
/// <param name="pan">左右の定位（例: -1.0で完全に左、0.0で中央、1.0で完全に右）</param>
void AudioManager::SetPanInternal(uint32_t handle, float pan) {
	auto voiceIt = voiceMap_.find(handle);
	auto dataIt = audioDataMap_.find(handle);
	if (voiceIt == voiceMap_.end() || dataIt == audioDataMap_.end() || !voiceIt->second) {
		return;
	}

	IXAudio2SourceVoice* voice = voiceIt->second;
	const AudioData& audioData = dataIt->second;

	pan = std::fmaxf(-1.0f, std::fminf(1.0f, pan));
	float leftVolume = (pan <= 0.0f) ? 1.0f : 1.0f - pan;
	float rightVolume = (pan >= 0.0f) ? 1.0f : 1.0f + pan;

	UINT32 srcChannels = audioData.waveFormat.nChannels;
	UINT32 dstChannels = 2;
	float outputMatrix[4] = {};

	if (srcChannels == 1) {
		outputMatrix[0] = leftVolume;
		outputMatrix[1] = rightVolume;
	} else if (srcChannels == 2) {
		outputMatrix[0] = leftVolume;
		outputMatrix[1] = 0.0f;
		outputMatrix[2] = 0.0f;
		outputMatrix[3] = rightVolume;
	} else {
		return;
	}

	voice->SetOutputMatrix(nullptr, srcChannels, dstChannels, outputMatrix);
}

//================================================================================
// 内部ヘルパー (ExtractAudioDataはそのまま)
//================================================================================
bool AudioManager::ExtractAudioData(const std::wstring& filePath, AudioData& outData) {
	HRESULT hr;

	ComPtr<IMFSourceReader> pReader;
	hr = MFCreateSourceReaderFromURL(filePath.c_str(), nullptr, &pReader);
	if (FAILED(hr)) return false;

	ComPtr<IMFMediaType> pPCMType;
	MFCreateMediaType(&pPCMType);
	pPCMType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	pPCMType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	hr = pReader->SetCurrentMediaType((DWORD) MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pPCMType.Get());
	if (FAILED(hr)) return false;

	ComPtr<IMFMediaType> pOutType;
	pReader->GetCurrentMediaType((DWORD) MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pOutType);

	WAVEFORMATEX* waveFormat = nullptr;
	MFCreateWaveFormatExFromMFMediaType(pOutType.Get(), &waveFormat, nullptr);
	outData.waveFormat = *waveFormat;
	CoTaskMemFree(waveFormat);

	while (true) {
		ComPtr<IMFSample> pSample;
		DWORD streamIndex = 0;
		DWORD flags = 0;
		LONGLONG llTimeStamp = 0;

		hr = pReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &streamIndex, &flags, &llTimeStamp, &pSample);

		if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
			break;
		}

		if (pSample) {
			ComPtr<IMFMediaBuffer> pBuffer;
			pSample->ConvertToContiguousBuffer(&pBuffer);

			BYTE* pData = nullptr;
			DWORD maxLength = 0;
			DWORD currentLength = 0;

			pBuffer->Lock(&pData, &maxLength, &currentLength);
			outData.audioBuffer.insert(outData.audioBuffer.end(), pData, pData + currentLength);
			pBuffer->Unlock();
		}
	}

	return true;
}