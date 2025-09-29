#pragma once

#include <cassert>
#include <wrl.h>
#include <xaudio2.h>

class SoundManager {
public:
  SoundManager();
  ~SoundManager();

  void Initialize();
  void Finalize();

  IXAudio2 *GetXAudio2() const { return xAudio2_.Get(); }

private:
  Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
  IXAudio2MasteringVoice *masterVoice_ = nullptr;
};
