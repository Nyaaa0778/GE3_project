#pragma once
#include "AudioManager.h"

#include <string>
#include <xapofx.h>
#include <xaudio2fx.h>

class AudioSource {
public:
	enum class FadeState {
		None,   // フェードしていない（通常状態）
		In,     // フェードイン中
		Out     // フェードアウト中
	};

	// 現在のフェード状態
	FadeState fadeState_ = FadeState::None;

public:
	AudioSource() = default;
	~AudioSource();

	// どの音声を鳴らすかセットする
	void SetAudio(const std::string& filepath);
	// 音量を変更する。0.0f(無音) ～ 1.0f(原音)。
	void SetVolume(float volume);
	// ピッチ（音の高さ・再生速度）を変更する。1.0f が通常。
	void SetPitch(float ratio);
	// 音のこもり具合（ぼかし）を設定する。
	// percentage: 0.0f(通常) ～ 1.0f(完全にこもる)
	void SetMuffle(float percentage);
	void SetPan(float pan);

	// 再生
	void PlayAudio(bool isLoop = false);
	// 一時停止
	void PauseAudio();   
	// 再開
	void ResumeAudio();
	// 停止
	void StopAudio();

	// 再生中かどうか判定する
	bool IsPlaying() const;

private:
	IXAudio2SourceVoice* sourceVoice_ = nullptr; // 自分のスピーカー
	const AudioData* audioData_ = nullptr; // 現在セットされているカセットテープ
};