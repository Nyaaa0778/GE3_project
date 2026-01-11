#pragma once

#include <cassert>
#include <string>
#include <wrl.h>
#include <xaudio2.h>
#include <vector>

struct SoundData {
  WAVEFORMATEX wfex;
  std::vector<BYTE> buffer;
};

class SoundManager {
public:
  SoundManager();
  ~SoundManager();

  void Initialize();
  void Finalize();

  bool Load(const std::string &filepath);
  void Play(SoundManager *soundManager);
  void Unload();

  IXAudio2 *GetXAudio2() const { return xAudio2_.Get(); }

private:
  SoundData soundData_{};
  IXAudio2SourceVoice *sourceVoice_ = nullptr;

  Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
  IXAudio2MasteringVoice *masterVoice_ = nullptr;
};
