#pragma once

#include <string>
#include <vector>
#include <wrl.h>
#include <memory>
#include <unordered_map>
#include <list>

#include <xaudio2.h>


//================================================================================
// 列挙型 / 構造体
//================================================================================

// 音のグループ
enum class SoundGroup {
	BGM,
	SE
};

struct AudioData {
	WAVEFORMATEX waveFormat = {};
	std::vector<BYTE> audioBuffer;
	SoundGroup group = SoundGroup::SE;
};

class AudioManager {
public:
	//================================================================================
	// シングルトン
	//================================================================================

	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns>DirectXCommonの唯一のインスタンス</returns>
	static AudioManager* GetInstance();

	/// <summary>
	/// コンストラクタ
	/// </summary>
	AudioManager() = default;
	/// <summary>
	/// デストラクタ
	/// </summary>
	~AudioManager();

	/// <summary>
	/// 終了
	/// </summary>
	static void Finalize();

private:

	static std::unique_ptr<AudioManager> instance;

	/// <summary>
	/// コピーコンストラクタ禁止
	/// </summary>
	/// <param name="">コピー元（使用不可）</param>
	AudioManager(AudioManager&) = delete;
	/// <summary>
	/// 代入演算子禁止
	/// </summary>
	/// <param name="">代入元（使用不可）</param>
	/// <returns>このオブジェクトを返す</returns>
	AudioManager& operator=(AudioManager&) = delete;

public:
	//================================================================================
	// 初期化・終了
	//================================================================================

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();
	/// <summary>
	/// 解放
	/// </summary>
	void Release();

	//================================================================================
	// 音声データ管理 (静的メンバ関数)
	//================================================================================

	// 音声ファイル読み込み 
	static uint32_t LoadAudio(const std::string& filepath, SoundGroup group = SoundGroup::SE);

	// メモリ解放
	static void Unload();

	//================================================================================
	// 音声操作 (静的メンバ関数)
	//================================================================================

	// 再生
	static void PlayAudio(uint32_t handle, bool isLoop = false);
	// ワンショット再生（同じ音を重複して鳴らす）
	static void PlayOneShot(uint32_t handle, float volume = 1.0f);
	// 一時停止
	static void PauseAudio(uint32_t handle);
	// 再開
	static void ResumeAudio(uint32_t handle);
	// 停止
	static void StopAudio(uint32_t handle);
	// 音が鳴っているか？
	static bool IsPlaying(uint32_t handle);

	// マスター音量
	static void SetMasterVolume(float volume);
	// BGMの音量
	static void SetBGMVolume(float volume);
	// SEの音量
	static void SetSEVolume(float volume);
	// ピッチ
	static void SetPitch(uint32_t handle, float ratio);
	// 音のこもり具合
	static void SetMuffle(uint32_t handle, float percentage);
	// パンニング
	static void SetPan(uint32_t handle, float pan);

private:
	//================================================================================
	// 音声データ管理
	//================================================================================

	/// <summary>
	/// 音声ファイルを読み込み、操作用のハンドル(ID)を返す
	/// </summary>
	/// <param name="filepath">音声ハンドル</param>
	/// <param name="group">音のグループ（SoundGroup::BGM / SE）</param>
	uint32_t LoadAudioInternal(const std::string& filepath, SoundGroup group = SoundGroup::SE);

	/// <summary>
	/// メモリを解放
	/// </summary>
	void UnloadInternal();

	//================================================================================
	// 音声操作
	//================================================================================

	/// <summary>
	/// 再生
	/// </summary>
	/// <param name="handle">音声ハンドル</param>
	/// <param name="isLoop">ループさせるか？</param>
	void PlayAudioInternal(uint32_t handle, bool isLoop = false);
	/// <summary>
	/// ワンショット再生（同じ音を重複して鳴らす）
	/// </summary>
	/// <param name="handle">音声ハンドル</param>
	/// <param name="volume">音量 (0.0f ~ 1.0f)</param>
	void PlayOneShotInternal(uint32_t handle, float volume);
	/// <summary>
	/// 一時停止
	/// </summary>
	/// <param name="handle">音声ハンドル</param>
	void PauseAudioInternal(uint32_t handle);
	/// <summary>
	/// 再開
	/// </summary>
	/// <param name="handle">音声ハンドル</param>
	void ResumeAudioInternal(uint32_t handle);
	/// <summary>
	/// 停止
	/// </summary>
	/// <param name="handle">音声ハンドル</param>
	void StopAudioInternal(uint32_t handle);
	/// <summary>
	/// 音を再生中かどうか
	/// </summary>
	/// <param name="handle">音声ハンドル</param>
	/// <returns></returns>
	bool IsPlayingInternal(uint32_t handle);

	/// <summary>
	/// マスター音量（最終出力）の設定
	/// </summary>
	/// <param name="volume">音量 (0.0f ~ 1.0f)</param>
	void SetMasterVolumeInternal(float volume);

	/// <summary>
	/// BGMの音量
	/// </summary>
	/// <param name="volume">設定する音量（例: 0.0で無音、1.0で標準音量）</param>
	void SetBGMVolumeInternal(float volume);
	/// <summary>
	/// SEの音量
	/// </summary>
	/// <param name="volume">設定する音量（例: 0.0で無音、1.0で標準音量）</param>
	void SetSEVolumeInternal(float volume);
	/// <summary>
	/// ピッチ
	/// </summary>
	/// <param name="handle">音声ハンドル</param>
	/// <param name="ratio">ピッチ（音程・再生速度）の倍率（例: 1.0で標準、2.0で1オクターブ上、0.5で1オクターブ下）</param>
	void SetPitchInternal(uint32_t handle, float ratio);
	/// <summary>
	/// 音のこもり具合
	/// </summary>
	/// <param name="handle">音声ハンドル</param>
	/// <param name="percentage">こもり具合（ローパスフィルター等）の割合（例: 0.0でクリア、1.0で最大）</param>
	void SetMuffleInternal(uint32_t handle, float percentage);
	/// <summary>
	/// パンニング
	/// </summary>
	/// <param name="handle">音声ハンドル</param>
	/// <param name="pan">左右の定位（例: -1.0で完全に左、0.0で中央、1.0で完全に右）</param>
	void SetPanInternal(uint32_t handle, float pan);

private:
	/// <summary>
	/// MFを用いた音声データの抽出処理
	/// </summary>
	bool ExtractAudioData(const std::wstring& filePath, AudioData& outData);

	//================================================================================
	// メンバ変数
	//================================================================================
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
	IXAudio2MasteringVoice* masterVoice_ = nullptr;

	// 次に発行するハンドル番号（0はエラー/未割り当て扱いにするため1から開始）
	uint32_t nextHandle_ = 1;

	// ファイルパスからハンドルを引く辞書（二重ロード防止用）
	std::unordered_map<std::string, uint32_t> pathToHandleMap_;

	// ハンドルから音声データを引く辞書
	std::unordered_map<uint32_t, AudioData> audioDataMap_;

	// ハンドルからボイス（スピーカー）を引く辞書
	std::unordered_map<uint32_t, IXAudio2SourceVoice*> voiceMap_;
	// ワンショット再生用の使い捨てボイスを管理するリスト
	std::list<IXAudio2SourceVoice*> oneShotVoices_;

	// グループごとのサブミックスボイス
	IXAudio2SubmixVoice* bgmSubmixVoice_ = nullptr;// BGM
	IXAudio2SubmixVoice* seSubmixVoice_ = nullptr; // SE
};