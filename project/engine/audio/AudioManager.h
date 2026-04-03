#pragma once

#include <cassert>
#include <string>
#include <vector>
#include <wrl.h>
#include <memory>
#include <unordered_map>
#include <xaudio2.h>

//================================================================================
// 構造体
//================================================================================
struct AudioData {
    WAVEFORMATEX waveFormat = {};
    std::vector<BYTE> audioBuffer;
};

class AudioManager {
public:
    //================================================================================
    // シングルトン
    //================================================================================

    // 唯一のインスタンス取得
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
    // 音声操作（図書館としての機能）
    //================================================================================

    /// <summary>
    ///  音声ファイル読み込み
    /// </summary>
    /// <param name="filepath">読み込む音声ファイル名</param>
    void LoadAudio(const std::string& filepath); 
    /// <summary>
    /// メモリを解放
    /// </summary>
    void Unload();

    //================================================================================
    // ゲッター（AudioSourceからアクセスするために必要）
    //================================================================================
    IXAudio2* GetXAudio2() const { return xAudio2_.Get(); }
    const AudioData* GetAudioData(const std::string& filepath) const;

private:

    /// <summary>
    /// MFを用いた音声データの抽出処理
    /// </summary>
    /// <param name="filePath"></param>
    /// <param name="outData"></param>
    /// <returns></returns>
    bool ExtractAudioData(const std::wstring& filePath, AudioData& outData);

    //================================================================================
    // メンバ変数
    //================================================================================
    Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
    IXAudio2MasteringVoice* masterVoice_ = nullptr;

    // 音声データを複数保持するための辞書（キャッシュ）
    std::unordered_map<std::string, AudioData> audioDatas_;
};