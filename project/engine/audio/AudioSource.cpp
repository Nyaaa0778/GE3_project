#include "AudioSource.h"

#include <cassert>
#include <cmath>

#pragma comment(lib, "xaudio2.lib")

#include "TimeManager.h"

AudioSource::~AudioSource() {
	StopAudio(); // 安全のため、破棄時に必ず音を止める

	if (sourceVoice_) {
		sourceVoice_->DestroyVoice();
		sourceVoice_ = nullptr;
	}
}

void AudioSource::SetAudio(const std::string& filepath) {
	if (sourceVoice_) {
		sourceVoice_->Stop();
		sourceVoice_->DestroyVoice();
		sourceVoice_ = nullptr;
	}

	// 【ここを追加！】マネージャーに「ロードしといて！」と裏でお願いする
	AudioManager::GetInstance()->LoadAudio(filepath);

	// 図書館(AudioManager)から音声データを借りてくる
	audioData_ = AudioManager::GetInstance()->GetAudioData(filepath);

	// データが見つからなければエラー
	assert(audioData_ && "指定された音声ファイルがロードされていません！");

	IXAudio2* xAudio2 = AudioManager::GetInstance()->GetXAudio2();
	HRESULT hr = xAudio2->CreateSourceVoice(&sourceVoice_, &audioData_->waveFormat, XAUDIO2_VOICE_USEFILTER);
	assert(SUCCEEDED(hr));
}

void AudioSource::SetVolume(float volume) {
	// スピーカー(sourceVoice_)が作られていない時は何もしない
	if (!sourceVoice_)
	{
		return;
	}

	// 音量が異常な値にならないよう、0.0f ～ 1.0f の間に制限（クランプ）する
	volume = std::fmaxf(0.0f, std::fminf(1.0f, volume));

	// XAudio2のボイスに音量を設定
	sourceVoice_->SetVolume(volume);
}

void AudioSource::SetPitch(float ratio) {
	if (sourceVoice_) {
		// XAUDIO2_MAX_FREQ_RATIO (通常1024.0f) を超えないように制限しつつ適用
		sourceVoice_->SetFrequencyRatio(ratio);
	}
}

void AudioSource::SetMuffle(float percentage) {
	if (!sourceVoice_)
	{
		return;
	}

	// percentage を 0.0f ～ 1.0f の間に制限
	percentage = std::fmaxf(0.0f, std::fminf(1.0f, percentage));

	XAUDIO2_FILTER_PARAMETERS params;
	params.Type = LowPassFilter; // ローパスフィルターを使う

	// --- 【ここがポイント！】カットオフ周波数の計算 ---
	// XAudio2のフィルターは 0.0f ～ 1.0f の「比率」で指定します。
	// 1.0f が全開放（通常音）、0.0f に近づくほど高音が削られます。
	// 人間の耳の特性上、単純な線形よりも、べき乗（pow）を使ったほうが
	// 自然なこもり具合の変化になります。

	// 1.0f(通常) から 0.01f(ほぼ無音) の間で反転させて計算
	params.Frequency = std::powf(1.0f - percentage, 3.0f);
	if (params.Frequency < 0.01f) params.Frequency = 0.01f; // 下限を設定

	// レゾナンス（特定の音域の強調）。今回はぼかしたいので、強調なしの最小値(1.0f)にする。
	params.OneOverQ = 1.0f;

	// ボイスにフィルター設定を適用
	sourceVoice_->SetFilterParameters(&params);
}

void AudioSource::SetPan(float pan) {
	if (!sourceVoice_ || !audioData_) {
		return;
	}

	// panの値を -1.0f ～ 1.0f の間に制限（クランプ）
	pan = std::fmaxf(-1.0f, std::fminf(1.0f, pan));

	// パンの値から左右のボリューム（0.0f ～ 1.0f）を計算する
	// 左に振った(pan < 0)場合は右の音量を下げ、右に振った(pan > 0)場合は左の音量を下げる
	float leftVolume = (pan <= 0.0f) ? 1.0f : 1.0f - pan;
	float rightVolume = (pan >= 0.0f) ? 1.0f : 1.0f + pan;

	// 音源のチャンネル数（モノラル=1, ステレオ=2）
	UINT32 srcChannels = audioData_->waveFormat.nChannels;
	// 出力先のチャンネル数（一般的なPC環境のステレオ出力を想定して2）
	UINT32 dstChannels = 2;

	// 出力行列（マトリックス）を作成
	// 最大で「入力2ch × 出力2ch = 4要素」あれば足りるので余裕を持って配列を用意
	float outputMatrix[4] = {};

	if (srcChannels == 1) {
		// モノラル音源の場合：1つの音を左右に割り振る
		outputMatrix[0] = leftVolume;  // [音源1] → [左スピーカー]
		outputMatrix[1] = rightVolume; // [音源1] → [右スピーカー]
	} else if (srcChannels == 2) {
		// ステレオ音源の場合：左の音は左に、右の音は右に出力しつつ全体のバランスを変える
		outputMatrix[0] = leftVolume;  // [音源左] → [左スピーカー]
		outputMatrix[1] = 0.0f;        // [音源右] → [左スピーカー] (混ざらないように0)
		outputMatrix[2] = 0.0f;        // [音源左] → [右スピーカー] (混ざらないように0)
		outputMatrix[3] = rightVolume; // [音源右] → [右スピーカー]
	} else {
		// 3ch以上の特殊なサラウンド音源などは今回処理しない
		return;
	}

	// 計算したマトリックスをボイスに適用する
	sourceVoice_->SetOutputMatrix(nullptr, srcChannels, dstChannels, outputMatrix);
}

void AudioSource::PlayAudio(bool isLoop) {
	if (!sourceVoice_ || !audioData_)
	{
		return;
	}

	// 最初から再生し直すために一旦止める
	sourceVoice_->Stop();
	sourceVoice_->FlushSourceBuffers();

	// バッファのセット
	XAUDIO2_BUFFER buf {};
	buf.pAudioData = audioData_->audioBuffer.data();
	buf.AudioBytes = static_cast<UINT32>(audioData_->audioBuffer.size());
	buf.Flags = XAUDIO2_END_OF_STREAM;

	if (isLoop) {
		buf.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	HRESULT hr = sourceVoice_->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(hr));

	hr = sourceVoice_->Start();
	assert(SUCCEEDED(hr));
}

void AudioSource::PauseAudio() {
	if (sourceVoice_) {
		// sourceVoice_->Stop(0) を呼ぶと、即座に音声の出力が止まります。
		// StopAudio() との違いは、FlushSourceBuffers() を「呼ばない」ことです。
		// これにより、再生位置のデータがメモリ内に保持されたままになります。
		sourceVoice_->Stop(0);
	}
}

void AudioSource::ResumeAudio() {
	if (sourceVoice_) {
		// Stop() した状態から Start() を呼ぶと、
		// キューに残っているデータの続きから自動的に再生が始まります。
		sourceVoice_->Start(0);
	}
}

void AudioSource::StopAudio() {
	if (sourceVoice_) {
		sourceVoice_->Stop();
		sourceVoice_->FlushSourceBuffers();
	}
}

bool AudioSource::IsPlaying() const {
	// スピーカーが無い場合は当然再生していない
	if (!sourceVoice_) {
		return false;
	}

	// ボイスの現在の状態を取得
	XAUDIO2_VOICE_STATE state;
	sourceVoice_->GetState(&state);

	// キューに積まれているバッファ（再生待ち・再生中のデータ）が1つ以上あれば再生中
	return state.BuffersQueued > 0;
}